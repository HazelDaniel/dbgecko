#include "include/storage.h"
#include "include/storage_s3.h"
#include "include/config_parser.h"
#include "include/tui.h"
#include <aws/auth/auth.h>
#include <aws/auth/signing_config.h>
#include <aws/common/common.h>
#include <aws/common/error.h>
#include <aws/common/logging.h>
#include <aws/common/math.h>
#include <aws/common/zero.h>
#include <aws/http/http.h>
#include <aws/io/io.h>
#include <aws/s3/s3.h>
#include <aws/sdkutils/sdkutils.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <threads.h>


/** -------------------------------------------------------------------------------------
 * Overview (s3 write):
 * write_open -> allocates buffer, sets multipart, may call create_multipart_upload
 * write_chunk -> streams data into part buffer; triggers upload_part when buffer fills
 * write_close -> completes multipart upload or does single PUT
 * write_abort -> cleanup if an error occurs.
 ----------------------------------------------------------------------------------------
 */

static _Bool aws_runtime_init = false;

static int s3_tui_logger_log(struct aws_logger *logger, enum aws_log_level log_level, aws_log_subject_t subject, const char *format, ...) {
  va_list args;
  char buf[BUF_LEN_L];
  TUIState_t *tui = get_tui_state();

  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  if (tui) {
    TUILogLevel_t t_lvl = LOG_INFO;
    if (log_level == AWS_LOG_LEVEL_ERROR || log_level == AWS_LOG_LEVEL_FATAL) t_lvl = LOG_ERROR;
    else if (log_level == AWS_LOG_LEVEL_WARN) t_lvl = LOG_WARN;

    tui_push_log(tui, t_lvl, "[s3] %s", buf);
  } else {
    fprintf(stderr, "[s3] %s\n", buf);
  }

  return AWS_OP_SUCCESS;
}

static enum aws_log_level s3_tui_logger_get_log_level(struct aws_logger *logger, aws_log_subject_t subject) {
#ifdef DEBUG_MODE
  return AWS_LOG_LEVEL_INFO;
#else
  return AWS_LOG_LEVEL_FATAL;
#endif
}

static void s3_tui_logger_clean_up(struct aws_logger *logger) {}

static struct aws_logger_vtable s3_tui_logger_vtable = {
  .log = s3_tui_logger_log,
  .get_log_level = s3_tui_logger_get_log_level,
  .clean_up = s3_tui_logger_clean_up,
};

S3InputStream_t *s3_input_stream_new(struct aws_allocator *allocator);
static void s3_meta_request_finish(struct aws_s3_meta_request *meta_request, const struct aws_s3_meta_request_result *result, void *user_data);

static void s3_meta_request_finish(struct aws_s3_meta_request *meta_request, const struct aws_s3_meta_request_result *result, void *user_data) {
  S3State_t *s = user_data;

  if (result->error_code != AWS_ERROR_SUCCESS) {
    fprintf(stderr, "[dbgecko] s3 meta-request finished with error %d: %s (%s)\n",
      result->error_code,
      aws_error_name(result->error_code),
      aws_error_debug_str(result->error_code));
    if (result->response_status) {
      fprintf(stderr, "[dbgecko] HTTP status: %d\n", result->response_status);
    }
    if (result->error_response_body && result->error_response_body->len > 0) {
      fprintf(stderr, "[dbgecko] Response body: %.*s\n",
        (int)result->error_response_body->len,
        (const char *)result->error_response_body->buffer);
    }
  }

  aws_mutex_lock(&s->mutex);

  if (result->error_code != AWS_ERROR_SUCCESS) s->failed = true;

  s->meta_request = NULL;
  aws_condition_variable_notify_all(&s->cv);

  aws_mutex_unlock(&s->mutex);

  aws_s3_meta_request_release(meta_request);
}

static void s3_tui_progress_callback(struct aws_s3_meta_request *meta_request, const struct aws_s3_meta_request_progress *progress, void *user_data) {
  TUIState_t *tui = get_tui_state();
  
  if (tui && progress->content_length > 0) {
    tui->progress_measurable = true;
    tui->progress = (float)progress->bytes_transferred / (float)progress->content_length;
  }
}
StorageStatus_t s3__write_open(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  S3State_t *s = ctx->state;
  struct aws_allocator *alloc = s->runtime->allocator;
  S3InputStream_t *in;
  int err_code;

  aws_mutex_init(&s->mutex);
  aws_condition_variable_init(&s->cv);

  s->buffer_cap = S3_PART_BUF_THRESHOLD;
  s->buffer_len = 0;
  s->buffer = aws_mem_calloc(alloc, 1, s->buffer_cap);

  if (!s->buffer) {
    set_err((const char **)err, BUF_LEN_XS, "out of memory");

    return STORAGE_WRITE_FAILED;
  }

  s->key = aws_byte_cursor_from_c_str(rel_path);
  s->total_bytes = 0;
  s->eof = false;
  s->failed = false;

  in = s3_input_stream_new(alloc);

  if (!in) {
    set_err((const char **)err, BUF_LEN_XS, "out of memory");
    aws_mem_release(alloc, s->buffer);

    return STORAGE_WRITE_FAILED;
  }

  in->local_state = s;

  s->stream = &in->base;

  struct aws_http_message *msg = aws_http_message_new_request(alloc);
  AppConfig_t *cfg = *get_app_config_handle();
  char path_buf[1024];
  const char *bd = cfg->storage->base_dir;
  while (*bd == '/') bd++;
  snprintf(path_buf, sizeof(path_buf), "/%s/%s", bd, rel_path);

  _Bool request_path_set_fail = aws_http_message_set_request_path(
    msg,
    aws_byte_cursor_from_c_str(path_buf)
  );

  if (request_path_set_fail) {
    set_err((const char **)err, BUF_LEN_XS, "failed to get the request destination path");

    return STORAGE_WRITE_FAILED;
  }

  aws_http_message_set_request_method(msg, aws_http_method_put);
  aws_http_message_set_body_stream(msg, s->stream);

  /* Set Host header for MinIO / S3-compatible endpoints */
  const struct aws_byte_cursor *host = aws_uri_host_name(&s->runtime->endpoint);
  uint32_t port = aws_uri_port(&s->runtime->endpoint);
  char host_buf[512];
  if (port > 0) {
    snprintf(host_buf, sizeof(host_buf), "%.*s:%u", (int)host->len, (const char *)host->ptr, port);
  } else {
    snprintf(host_buf, sizeof(host_buf), "%.*s", (int)host->len, (const char *)host->ptr);
  }

  struct aws_http_header host_header = {
    .name = aws_byte_cursor_from_c_str("Host"),
    .value = aws_byte_cursor_from_c_str(host_buf),
  };
  aws_http_message_add_header(msg, host_header);

  struct aws_s3_meta_request_options opts = {
    .type = AWS_S3_META_REQUEST_TYPE_PUT_OBJECT,
    .message = msg,
    .signing_config = s->runtime->signing,
    .endpoint = &s->runtime->endpoint,
    .user_data = s,
    .finish_callback = s3_meta_request_finish,
    .body_callback = NULL,
    .headers_callback = NULL,
    .progress_callback = s3_tui_progress_callback,
    .shutdown_callback = NULL,
  };

  s->meta_request = aws_s3_client_make_meta_request(s->runtime->client, &opts);

  if (!s->meta_request) {
    err_code = aws_last_error();
    set_err((const char **)err, BUF_LEN_SS, "failed to create s3 request (%s-> %s)", aws_error_name(err_code), aws_error_debug_str(err_code));
    aws_http_message_release(msg);

    return STORAGE_WRITE_FAILED;
  }

  aws_http_message_release(msg);

  return STORAGE_OK;
}

StorageStatus_t s3__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len, StorageErrorMessage_t *err) {
  S3State_t *s = ctx->state;
  const uint8_t *p = buf;
  size_t remaining = len;

  aws_mutex_lock(&s->mutex);

  while (remaining > 0) {

    while (s->buffer_len == s->buffer_cap && !s->failed) aws_condition_variable_wait(&s->cv, &s->mutex);

    if (s->failed) {
      aws_mutex_unlock(&s->mutex);

      return STORAGE_WRITE_FAILED;
    }

    size_t space = s->buffer_cap - s->buffer_len;
    size_t to_copy = aws_min_size(remaining, space);

    memcpy(s->buffer + s->buffer_len, p, to_copy);

    s->buffer_len += to_copy;
    s->total_bytes += to_copy;

    p += to_copy;
    remaining -= to_copy;

    aws_condition_variable_notify_one(&s->cv);
  }

  aws_mutex_unlock(&s->mutex);

  return STORAGE_OK;
}

StorageStatus_t s3__write_close(const StorageContext_t *ctx, StorageErrorMessage_t *err) {
  S3State_t *s = ctx->state;
  _Bool failed = false;

  aws_mutex_lock(&s->mutex);

  s->eof = true;
  aws_condition_variable_notify_all(&s->cv);

  /* Wait for meta request to finish */
  while (s->meta_request != NULL && !s->failed) aws_condition_variable_wait(&s->cv, &s->mutex);

  failed = s->failed;

  aws_mutex_unlock(&s->mutex);

  aws_mem_release(s->runtime->allocator, s->buffer);
  s->buffer = NULL;
  aws_mutex_clean_up(&s->mutex);
  aws_condition_variable_clean_up(&s->cv);

  return failed ? STORAGE_WRITE_FAILED : STORAGE_OK;
}

StorageStatus_t s3__write_abort(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  return status;
}

static int part_stream_read(struct aws_input_stream *stream, struct aws_byte_buf *dest) {
  S3PartStream_t *s = AWS_CONTAINER_OF(stream, S3PartStream_t, base);
  size_t remaining = s->len - s->offset;
  size_t space = dest->capacity - dest->len;
  size_t to_copy = aws_min_size(space, remaining);

  memcpy(dest->buffer + dest->len, s->buf + s->offset, to_copy);
  dest->len += to_copy;
  s->offset += to_copy;

  return AWS_OP_SUCCESS;
}

static int s3_input_stream_read(struct aws_input_stream *stream, struct aws_byte_buf *dest) {
  S3InputStream_t *in = AWS_CONTAINER_OF(stream, S3InputStream_t, base);
  S3State_t *s = in->local_state;
  size_t to_copy;

  aws_mutex_lock(&s->mutex);

  while (s->buffer_len == 0 && !s->eof && !s->failed) aws_condition_variable_wait(&s->cv, &s->mutex);

  if (s->failed) {
    aws_mutex_unlock(&s->mutex);

    return aws_raise_error(AWS_ERROR_UNKNOWN);
  }

  if (s->buffer_len == 0 && s->eof) {
    /* EOF — don't touch dest, just return success */
    aws_mutex_unlock(&s->mutex);

    return AWS_OP_SUCCESS;
  }

  size_t space = dest->capacity - dest->len;
  to_copy = space < s->buffer_len ? space : s->buffer_len;

  memcpy(dest->buffer + dest->len, s->buffer, to_copy);
  memmove(s->buffer, s->buffer + to_copy, s->buffer_len - to_copy);

  s->buffer_len -= to_copy;

  dest->len += to_copy;

  // Wake producer
  aws_condition_variable_notify_one(&s->cv);

  aws_mutex_unlock(&s->mutex);

  return AWS_OP_SUCCESS;
}

static int s3_get_body_cb(struct aws_s3_meta_request *req, const struct aws_byte_cursor *body, uint64_t range_start, void *user_data) {
  S3ReadState_t *st = user_data;
  ssize_t n = st->sink(st->sink_ud, body->ptr, body->len, &st->last_status);

  if (n < 0 || (size_t)n != body->len)
    return AWS_OP_ERR;

  return AWS_OP_SUCCESS;
}

static void s3_get_finished(struct aws_s3_meta_request *req, const struct aws_s3_meta_request_result *result, void *user_data) {
  S3ReadState_t *st = user_data;

  if (result->error_code != AWS_ERROR_SUCCESS) {
    fprintf(stderr, "[dbgecko] s3 GET finished with error %d: %s (%s)\n",
      result->error_code,
      aws_error_name(result->error_code),
      aws_error_debug_str(result->error_code));
    if (result->response_status) {
      fprintf(stderr, "[dbgecko] HTTP status: %d\n", result->response_status);
    }
    if (result->error_response_body && result->error_response_body->len > 0) {
      fprintf(stderr, "[dbgecko] Response body: %.*s\n",
        (int)result->error_response_body->len,
        (const char *)result->error_response_body->buffer);
    }
  }

  aws_mutex_lock(&st->mutex);

  st->error_code = result->error_code;
  st->done = true;

  aws_condition_variable_notify_one(&st->cv);
  aws_mutex_unlock(&st->mutex);

  aws_s3_meta_request_release(req);
}

static int s3_wait_for_read(S3ReadState_t *st) {
  int err;

  aws_mutex_lock(&st->mutex);
  while (!st->done)
    aws_condition_variable_wait(&st->cv, &st->mutex);
  err = st->error_code;
  aws_mutex_unlock(&st->mutex);

  return err;
}

StorageStatus_t s3__read_file(const StorageContext_t *ctx, const char *rel_path, StorageDataSink sink, void *sink_ud, StorageErrorMessage_t *err) {
  S3State_t *s = ctx->state;
  struct aws_allocator *alloc = s->runtime->allocator;
  int rc;

  S3ReadState_t st = {
    .sink = sink,
    .sink_ud = sink_ud,
    .last_status = STORAGE_OK,
    .done = false,
    .error_code = AWS_ERROR_SUCCESS,
  };

  aws_mutex_init(&st.mutex);
  aws_condition_variable_init(&st.cv);

  AppConfig_t *cfg = *get_app_config_handle();
  char path_buf[1024];
  const char *bd = cfg->storage->base_dir;
  while (*bd == '/') bd++;
  snprintf(path_buf, sizeof(path_buf), "/%s/%s", bd, rel_path);

  struct aws_http_message *msg = aws_http_message_new_request(alloc);
  rc = aws_http_message_set_request_method(msg, aws_http_method_get);
  assert(rc == AWS_OP_SUCCESS);
  rc = aws_http_message_set_request_path(msg, aws_byte_cursor_from_c_str(path_buf));
  assert(rc == AWS_OP_SUCCESS);

  /* Set Host header for MinIO / S3-compatible endpoints */
  const struct aws_byte_cursor *host = aws_uri_host_name(&s->runtime->endpoint);
  uint32_t port = aws_uri_port(&s->runtime->endpoint);
  char host_buf[512];
  if (port > 0) {
    snprintf(host_buf, sizeof(host_buf), "%.*s:%u", (int)host->len, (const char *)host->ptr, port);
  } else {
    snprintf(host_buf, sizeof(host_buf), "%.*s", (int)host->len, (const char *)host->ptr);
  }

  struct aws_http_header host_header = {
    .name = aws_byte_cursor_from_c_str("Host"),
    .value = aws_byte_cursor_from_c_str(host_buf),
  };
  aws_http_message_add_header(msg, host_header);

  struct aws_s3_meta_request_options opts = {
    .type = AWS_S3_META_REQUEST_TYPE_GET_OBJECT,
    .message = msg,
    .body_callback = s3_get_body_cb,
    .finish_callback = s3_get_finished,
    .progress_callback = s3_tui_progress_callback,
    .user_data = &st,
    .signing_config = s->runtime->signing,
    .endpoint = &s->runtime->endpoint,
  };

  struct aws_s3_meta_request *req = aws_s3_client_make_meta_request(s->runtime->client, &opts);

  aws_http_message_release(msg);

  if (!req) {
    set_err((const char **)err, BUF_LEN_XS, "failed to create s3 get request");

    return STORAGE_READ_FAILED;
  }

  int aws_err = s3_wait_for_read(&st);

  aws_mutex_clean_up(&st.mutex);
  aws_condition_variable_clean_up(&st.cv);

  if (aws_err != AWS_ERROR_SUCCESS || st.last_status != STORAGE_OK) {
    set_err((const char **)err, BUF_LEN_XS, "s3 read failed: %s", aws_error_debug_str(aws_err));

    return STORAGE_READ_FAILED;
  }

  return STORAGE_OK;
}

StorageStatus_t s3__mkdir (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 running mkdir aiit\n");

  return status;
}

StorageStatus_t s3__delete_file (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 deleting file aiit\n");

  return status;
}

_Bool s3__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 file exists aiit\n");

  return status;
}

struct aws_credentials_provider *get_s3_runtime_credentials_provider(struct aws_allocator *alloc) {
  AppConfig_t *cfg = *get_app_config_handle();
  S3Config_t s3_cfg;
  s3_cfg = cfg->storage->backend->backend.s3;

  struct aws_credentials_provider_static_options cred_provider_static;
  AWS_ZERO_STRUCT(cred_provider_static);
  cred_provider_static.access_key_id = aws_byte_cursor_from_c_str(s3_cfg.access_key);
  cred_provider_static.secret_access_key = aws_byte_cursor_from_c_str(s3_cfg.secret_key);

  if (s3_cfg.session_token && strcmp(s3_cfg.session_token, ".") != 0 && s3_cfg.session_token[0] != '\0') {
    cred_provider_static.session_token = aws_byte_cursor_from_c_str(s3_cfg.session_token);
  }

  struct aws_credentials_provider *creds = aws_credentials_provider_new_static(alloc, &cred_provider_static);

  return creds;
}

S3Runtime_t *init_s3_runtime() {
  S3Runtime_t *rt = aws_mem_calloc(aws_default_allocator(), 1, sizeof(*rt));
  if (!rt) return NULL;

  struct aws_allocator *alloc = aws_default_allocator();
  AppConfig_t *cfg = *get_app_config_handle();

  if (!aws_runtime_init) {
    // aws_sdkutils_library_init(alloc);
    aws_common_library_init(alloc);
    aws_io_library_init(alloc);
    aws_http_library_init(alloc);
    aws_auth_library_init(alloc);
    aws_s3_library_init(alloc);

    aws_runtime_init = true;
  }

  struct aws_logger *logger = aws_mem_calloc(alloc, 1, sizeof(struct aws_logger));

  if (!logger) {
    destroy_s3_runtime(rt);
    return NULL;
  }

  logger->allocator = alloc;
  logger->vtable = &s3_tui_logger_vtable;

  aws_logger_set(logger);

  rt->allocator = alloc;
  rt->elg = aws_event_loop_group_new_default(alloc, AWS_S3_CPU_MIN_COUNT, NULL);
  rt->creds = get_s3_runtime_credentials_provider(alloc);

  struct aws_host_resolver_default_options resolver_opts = {
    .max_entries = AWS_S3_HOST_MAX_RETRIES,
    .el_group = rt->elg,
  };

  rt->resolver = aws_host_resolver_new_default(alloc, &resolver_opts);

  struct aws_client_bootstrap_options bst_opts = {
    .event_loop_group = rt->elg,
    .host_resolver = rt->resolver,
  };

  rt->bootstrap = aws_client_bootstrap_new(alloc, &bst_opts);

  rt->signing = aws_mem_calloc(alloc, 1, sizeof(struct aws_signing_config_aws));

  if (!rt->signing) {
    destroy_s3_runtime(rt);
    return NULL;
  }

  rt->signing->config_type = AWS_SIGNING_CONFIG_AWS;
  rt->signing->algorithm = AWS_SIGNING_ALGORITHM_V4;
  rt->signing->signature_type = AWS_ST_HTTP_REQUEST_HEADERS;
  rt->signing->service = aws_byte_cursor_from_c_str("s3");
  rt->signing->region = aws_byte_cursor_from_c_str(cfg->storage->backend->backend.s3.region);
  rt->signing->credentials_provider = rt->creds;
  rt->signing->signed_body_header = AWS_SBHT_X_AMZ_CONTENT_SHA256;

  /* Parse endpoint URI from config for MinIO / S3-compatible services */
  const char *endpoint_str = cfg->storage->backend->backend.s3.endpoint;
  struct aws_byte_cursor endpoint_cursor = aws_byte_cursor_from_c_str(endpoint_str);
  if (aws_uri_init_parse(&rt->endpoint, alloc, &endpoint_cursor) != AWS_OP_SUCCESS) {
    destroy_s3_runtime(rt);
    return NULL;
  }

  /* Determine TLS mode from the endpoint scheme (http:// vs https://) */
  const struct aws_byte_cursor *scheme = aws_uri_scheme(&rt->endpoint);
  _Bool use_tls = (scheme->len == 5 && memcmp(scheme->ptr, "https", 5) == 0);

  struct aws_s3_client_config client_config;
  AWS_ZERO_STRUCT(client_config);

  client_config.client_bootstrap = rt->bootstrap;
  client_config.region = aws_byte_cursor_from_c_str(cfg->storage->backend->backend.s3.region);
  client_config.tls_mode = use_tls ? AWS_MR_TLS_ENABLED : AWS_MR_TLS_DISABLED;
  client_config.signing_config = rt->signing;

  rt->client = aws_s3_client_new(alloc, &client_config);

  if (!rt->client) {
    destroy_s3_runtime(rt);

    return NULL;
  }

  assert(rt->creds);
  assert(rt->client);
  assert(rt->bootstrap);
  assert(rt->elg);

  return rt;
}

void destroy_s3_runtime(S3Runtime_t *rt) {
  // order matters :)
  if (!rt || !aws_runtime_init) return;
  if (rt->client) aws_s3_client_release(rt->client);
  if (rt->bootstrap) aws_client_bootstrap_release(rt->bootstrap);
  if (rt->resolver) aws_host_resolver_release(rt->resolver);
  if (rt->creds) aws_credentials_provider_release(rt->creds);
  if (rt->elg) aws_event_loop_group_release(rt->elg);
  if (rt->signing) aws_mem_release(rt->allocator, rt->signing);
  rt->signing = NULL;
  aws_uri_clean_up(&rt->endpoint);

  aws_mem_release(rt->allocator, rt);

  // if (aws_runtime_init) {
  //   aws_s3_library_clean_up();
  //   aws_auth_library_clean_up();
  //   aws_http_library_clean_up();
  //   aws_io_library_clean_up();
  //   aws_common_library_clean_up();
  // }
}

static int s3_input_stream_get_status(struct aws_input_stream *stream, struct aws_stream_status *status) {
  S3InputStream_t *in = AWS_CONTAINER_OF(stream, S3InputStream_t, base);
  StorageContext_t *ctx = get_storage_context_from_protocol(PTC_S3);
  S3State_t *s = ctx->state;

  aws_mutex_lock(&s->mutex);

  status->is_end_of_stream = (s->eof && s->buffer_len == 0);
  status->is_valid = !s->failed;

  aws_mutex_unlock(&s->mutex);

  return AWS_OP_SUCCESS;
}

static int s3_input_stream_seek(struct aws_input_stream *stream, int64_t offset, enum aws_stream_seek_basis basis) {
  (void)stream;
  (void)offset;
  (void)basis;

  return aws_raise_error(AWS_ERROR_UNSUPPORTED_OPERATION);
}

static void s3_input_stream_acquire(struct aws_input_stream *stream) {
  S3InputStream_t *in = AWS_CONTAINER_OF(stream, S3InputStream_t, base);

  aws_ref_count_acquire(&in->base.ref_count);
}

static void s3_input_stream_release(struct aws_input_stream *stream) {
  S3InputStream_t *s = stream->impl;

  if (aws_ref_count_release(&s->base.ref_count)) {
    aws_mem_release(s->allocator, s);
  }
}

static struct aws_input_stream_vtable s3_input_stream_vtable = {
  .read = s3_input_stream_read,
  .seek = s3_input_stream_seek,
  .get_status = s3_input_stream_get_status,
  .get_length = NULL,
  .acquire = NULL,
  .release = NULL,
};

static void s3_input_stream_destroy(void *object) {
  S3InputStream_t *s = object;

  aws_mem_release(s->allocator, s);
}

S3InputStream_t *s3_input_stream_new(struct aws_allocator *allocator) {
  S3InputStream_t *in = aws_mem_calloc(allocator, 1, sizeof(*in));

  if (!in) return NULL;

  in->allocator = allocator;
  in->base.vtable = &s3_input_stream_vtable;
  in->base.impl = in;

  aws_ref_count_init(&in->base.ref_count, in, s3_input_stream_destroy);

  return in;
}
