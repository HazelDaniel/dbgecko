#include <stdio.h>
#include <unistd.h>
#include "include/storage.h"
#include "include/config_parser.h"
#include "include/storage_s3.h"
#include "include/storage_ssh.h"
#include "include/storage_sftp.h"
#include "include/storage_local.h"
#include "include/plugin_context.h"


StackStatus_t destroy_local_ssh_state(SSHState_t *state_ssh) {
  StackStatus_t status = EXEC_SUCCESS;

  if (state_ssh->session) ssh_free(state_ssh->session);
  free(state_ssh);

  return status;
}

/* -----------------------S3-------------------------------- */
S3State_t *create_s3_state() {
  S3State_t *state = malloc(sizeof(S3State_t));

  if (!state) return NULL;

  state->http_client = NULL;
  state->multipart_upload_id = NULL;

  return state;
}

StackStatus_t cleanup_s3_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  S3State_t *state_s3 = (S3State_t *)ctx->state;

  if (!state_s3) return EXEC_FAILURE;

  if (state_s3->http_client) free(state_s3->http_client);
  if (state_s3->multipart_upload_id) free(state_s3->multipart_upload_id);
  free(state_s3);

  return status;
}

StorageContext_t *create_s3_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();

  ctx->cleanup = cleanup_s3_state;
  ctx->ops = &storage_ops_table[PTC_S3];
  ctx->state = create_s3_state();

  return ctx;
}
/* ----------------------------------------------------------- */

/* -----------------------SFTP-------------------------------- */
SFTPState_t *create_sftp_state() {
  SFTPState_t *state = malloc(sizeof(SFTPState_t));
  AppConfig_t *cfg = *get_app_config_handle();
  SFTPConfig_t sftp;

  if (!state) return NULL;
  if (!cfg) return NULL;

  sftp = cfg->storage->backend->backend.sftp;

  // todo: read 'host' key for both ssh and sftp from config/cli
  state->parent_state = create_ssh_state(state->private_key, state->host, state->username,
    state->port, state->max_retries, state->timeout_seconds, false);

  if (!state->parent_state) return NULL;

  state->max_retries = sftp.max_retries;
  state->port = sftp.port;
  state->timeout_seconds = sftp.timeout_seconds;
  snprintf(state->username, BUF_LEN_S, "%s", sftp.username);
  snprintf(state->host, BUF_LEN_XS, "%s", sftp.host);
  snprintf(state->private_key, BUF_LEN_S, "%s", sftp.private_key);
  state->session = sftp_new(state->parent_state->session);

  return state;
}

StackStatus_t cleanup_sftp_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  SFTPState_t *state_sftp = (SFTPState_t *)ctx->state;

  if (!state_sftp) return EXEC_FAILURE;

  if (state_sftp->parent_state) {
    status = destroy_local_ssh_state(state_sftp->parent_state);

    if (status != EXEC_SUCCESS) {

      sftp_free(state_sftp->session);
      free(state_sftp);

      return status;
    }
  }

  sftp_free(state_sftp->session);
  free(state_sftp);

  return status;
}

StorageContext_t *create_sftp_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();

  ctx->cleanup = cleanup_sftp_state;
  ctx->ops = &storage_ops_table[PTC_SFTP];
  ctx->state = create_sftp_state();

  return ctx;
}
/* ----------------------------------------------------------- */

/* -----------------------SSH--------------------------------- */
SSHState_t *create_ssh_state(const char *private_key, const char *host, const char *username,
  size_t port, size_t max_retries, size_t timeout_seconds, bool verify_known_hosts) {
  SSHState_t *state = malloc(sizeof(SSHState_t));

  if (!state) return NULL;

  state->max_retries = max_retries;
  state->port = port;
  state->verify_known_hosts = verify_known_hosts;
  state->timeout_seconds = timeout_seconds;
  snprintf(state->username, BUF_LEN_S, "%s", username);
  snprintf(state->host, BUF_LEN_XS, "%s", host);
  snprintf(state->private_key, BUF_LEN_S, "%s", private_key);
  state->session = ssh_new();

  // ssh_options_set(state->session, SSH_OPTIONS_ADD_IDENTITY, private_key);
  //// ssh_options_set(state->session, SSH_OPTIONS_BINDADDR, "");
  //// ssh_options_set(state->session, SSH_OPTIONS_COMPRESSION, "");
  // ssh_options_set(state->session, SSH_OPTIONS_USER, username);
  // ssh_options_set(state->session, SSH_OPTIONS_HOST, host);
  // ssh_options_set(state->session, SSH_OPTIONS_TIMEOUT, &timeout_seconds);
  // ssh_options_set(state->session, SSH_OPTIONS_PORT, &port);
  // ssh_options_set(state->session, SSH_OPTIONS_STRICTHOSTKEYCHECK, "true");

  return state;
}

StackStatus_t cleanup_ssh_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  SSHState_t *state_ssh = (SSHState_t *)(ctx->state);

  if (!state_ssh) return EXEC_FAILURE;

  status = destroy_local_ssh_state(state_ssh);

  return status;
}

StorageContext_t *create_ssh_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();
  AppConfig_t *cfg = *get_app_config_handle();
  SSHConfig_t ssh;

  if (!cfg) return NULL;

  ssh = cfg->storage->backend->backend.ssh;

  ctx->cleanup = cleanup_ssh_state;
  ctx->ops = &storage_ops_table[PTC_SSH];
  ctx->state = create_ssh_state(ssh.private_key, ssh.host, ssh.username, ssh.port, ssh.max_retries,
    ssh.timeout_seconds, ssh.verify_known_hosts);

  if (!ctx->state) {
    ctx->cleanup = NULL, ctx->ops = NULL, ctx->state = NULL;
    free(ctx);

    return NULL;
  }

  return ctx;
}
/* ----------------------------------------------------------- */

/* -----------------------LOCAL-------------------------------- */
LocalFSState_t *create_local_fs_state() {
  LocalFSState_t *state = malloc(sizeof(LocalFSState_t));

  if (!state) return NULL;

  state->bytes_transferred = 0;
  state->fd = -1;

  snprintf(state->final_path, 1, "");
  snprintf(state->tmp_path, 1, "");

  return state;
}

StackStatus_t cleanup_local_fs_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  LocalFSState_t *state_local = (LocalFSState_t *)ctx->state;

  if (!state_local) return EXEC_FAILURE;

  if (state_local->fd >= 0) {
    close(state_local->fd);
    state_local->fd = -1;
  }

  free(state_local);

  return status;
}

StorageContext_t *create_local_fs_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();

  ctx->cleanup = cleanup_local_fs_state;
  ctx->ops = &storage_ops_table[PTC_LOCAL];
  ctx->state = create_local_fs_state();

  return ctx;
}
/* ----------------------------------------------------------- */

StackStatus_t destroy_storage_context(StorageContext_t *ctx) {
  StackStatus_t status = EXEC_SUCCESS;

  ctx->cleanup(ctx);
  free(ctx);

 return status;
}

RemoteStorageProtocol_t extract_protocol_from_uri(const char *uri) {
  RemoteStorageProtocol_t ptc = PTC_UNKNOWN;
  char uri_cp[BUF_LEN_S] = {0};
  const char *token = NULL;

  strcpy(uri_cp, uri);
  token = strtok(uri_cp, ":");

  if (strcmp(token, SSH_PTC_TOKEN) == 0) return PTC_SSH;
  if (strcmp(token, SFTP_PTC_TOKEN) == 0) return PTC_SFTP;
  // TODO: use regex to parse "user@host:..." for ssh
  if (strcmp(token, S3_PTC_TOKEN) == 0) return  PTC_S3;

  return ptc;
}

static StorageContext_t *context_lookup[SUPPORTED_PROTOCOL_COUNT] = {
  [PTC_S3] = NULL,
  [PTC_SSH] = NULL,
  [PTC_SFTP] = NULL,
  [PTC_LOCAL] = NULL,
};

StorageContext_t *get_storage_context_from_protocol(RemoteStorageProtocol_t ptc) {
  switch (ptc) {
    case PTC_S3: {
      if (!context_lookup[ptc]) context_lookup[ptc] = create_s3_context();
      return context_lookup[ptc];
    };
    case PTC_SSH: {
      if (!context_lookup[ptc]) context_lookup[ptc] = create_ssh_context();
      return context_lookup[ptc];
    }
    case PTC_SFTP: {
      if (!context_lookup[ptc]) context_lookup[ptc] = create_sftp_context();
      return context_lookup[ptc];
    }
    case PTC_LOCAL: {
      if (!context_lookup[ptc]) context_lookup[ptc] = create_local_fs_context();
      return context_lookup[ptc];
    }
    default: return NULL;
  }
}

RemoteStorageProtocol_t ___unsafe_to_ptc___(size_t obj) {
  return (RemoteStorageProtocol_t)obj;
}

/**
 * storage_write_stream - storage/transport backend agnostic write streamer
 * @ctx: storage context for a storage backend
 * @dst_path: the output path to which the stream is atomically written eventually
 * @source: a callback function provided by db plugins - internal to each plugin
 * @local_state: the state for the source function
 * @err: the error message to propagate (in the event of a fail)
 * Returns: StorageStatus_t
 */
StorageStatus_t storage_write_stream(StorageContext_t *ctx, const char *dst_path, StorageDataSource source,
  void *local_state, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;
  uint8_t buffer[DEFAULT_STREAM_BUFFER_SIZE];
  char tmp_path[BUF_LEN_S];
  StorageStatus_t read_status = STORAGE_OK;
  ssize_t n;

  if (!ctx || !ctx->ops || !ctx->ops->write_open ||
    !ctx->ops->write_chunk || !ctx->ops->write_close || !ctx->ops->write_abort) {
    set_err((const char **)err, BUF_LEN_XS, "Invalid or missing storage ops");

    return STORAGE_WRITE_FAILED;
  }

  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", dst_path);

  status = ctx->ops->write_open(ctx, tmp_path, err);
  if (status != STORAGE_OK) return status;

  for (;;) {
    read_status = STORAGE_OK;
    n = source(local_state, buffer, sizeof(buffer), &read_status);

    if (read_status != STORAGE_OK) {
      status = read_status;
      goto fail_cleanup;
    }

    if (n == 0) { /* EOF */
      break;
    }

    if (n < 0) {
      status = STORAGE_ERR_IO;
      set_err((const char **)err, BUF_LEN_XS, "Data source returned -1");
      goto fail_cleanup;
    }

    status = ctx->ops->write_chunk(ctx, buffer, (size_t)n, err);

    if (status != STORAGE_OK) {
      goto fail_cleanup;
    }
  }

  status = ctx->ops->write_close(ctx, tmp_path, dst_path, err);

  return status;

fail_cleanup:
  ctx->ops->write_abort(ctx, tmp_path, err);
  return status;
}

/**
 * storage_read_stream - storage/transport backend agnostic read streamer
 * @ctx: storage context for a storage backend
 * @src_path: object path in storage (relative path)
 * @sink: a callback function provided by db plugins - consumer to receive chunks
 * @local_state: the state for the sink function
 * @err: the error message to propagate (in the event of a fail)
 * Returns: StorageStatus_t
 */
StorageStatus_t storage_read_stream(StorageContext_t *ctx, const char *src_path, StorageDataSink sink,
  void *local_state, StorageErrorMessage_t *err) {
  if (!ctx || !ctx->state || !src_path || !sink) {
    set_err((const char **)err, BUF_LEN_XS, "invalid args");
    return STORAGE_READ_FAILED;
  }

  return local_fs__read_file(ctx, src_path, sink, local_state, err);
}
