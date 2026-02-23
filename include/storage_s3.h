#ifndef ___STORAGE_S3_H___
#define ___STORAGE_S3_H___

// external library headers
#include <aws/auth/signing_config.h>
#include <aws/common/allocator.h>
#include <aws/common/byte_buf.h>
#include <aws/common/common.h>                      // core utilities
#include <aws/common/mutex.h>                      // core/multi-threading
#include <aws/common/condition_variable.h>        // core/multi-threading
#include <aws/common/uri.h>
#include <aws/http/request_response.h>
#include <aws/io/io.h>                // networking
#include <aws/io/stream.h>            // networking streaming
#include <aws/io/event_loop.h>        // networking internals
#include <aws/io/channel_bootstrap.h> // networking internals
#include <aws/cal/cal.h>
#include <aws/auth/credentials.h>
#include <aws/http/connection.h>
#include <aws/s3/s3.h>
#include <aws/s3/s3_buffer_pool.h>
#include <aws/s3/s3_client.h>
#include <aws/common/byte_buf.h>
#include <aws/common/error.h>
#include <aws/common/math.h>

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"

// macro defs
#define S3_PART_BUF_THRESHOLD (BUF_LEN * BUF_LEN) // 1mb


typedef struct S3Runtime {
  struct aws_event_loop_group         *elg;
  struct aws_host_resolver            *resolver;
  struct aws_client_bootstrap         *bootstrap;
  struct aws_credentials_provider     *creds;
  struct aws_s3_client                *client;
  struct aws_allocator                *allocator;
  struct aws_signing_config_aws       *signing;
  struct aws_uri                      endpoint;
} S3Runtime_t;

struct aws_input_stream;

typedef struct S3State {
  /* per-write operation */

  struct aws_byte_cursor        key; // resolved S3 object key (bucket/path)
  uint8_t                       *buffer;
  size_t                        buffer_len;
  size_t                        buffer_cap;
  struct aws_input_stream       *stream;
  /* bookkeeping */
  size_t                        total_bytes;

  /* synchronization and status report */
  _Bool                         eof;
  _Bool                         failed;
  struct aws_mutex              mutex;
  struct aws_condition_variable cv;

  S3Runtime_t                   *runtime;
  struct aws_s3_meta_request    *meta_request;
} S3State_t;

typedef struct s3StreamState {
  uint8_t               *buffer;
  size_t                buffer_len;
  size_t                read_offset;
  _Bool                 closed;
  struct aws_allocator  *allocator;
} s3StreamState_t;

typedef struct S3InputStream {
  struct aws_input_stream   base;
  StorageDataSource         source;
  void                      *local_state;
  StorageStatus_t           last_status;
  bool                      eof;
  struct aws_allocator      *allocator;
} S3InputStream_t;

typedef struct S3ReadState {
  struct aws_mutex                mutex;
  struct aws_condition_variable   cv;
  bool                            done;
  int                             error_code;

  StorageDataSink                 sink;
  void                            *sink_ud;
  StorageStatus_t                 last_status;
} S3ReadState_t;

typedef struct S3PartStream {
  struct aws_input_stream     base;
  uint8_t                     *buf;
  size_t                      len;
  size_t                      offset;
  struct aws_allocator        *alloc;
} S3PartStream_t;

typedef struct s3MultipartInitResult {
  struct aws_mutex                mutex;
  struct aws_condition_variable   cv;
  bool                            done;
  int                             error_code;
  struct aws_string               *upload_id;
} s3MultipartInitResult_t;

S3State_t *create_s3_state();

StorageContext_t *create_s3_context();

static struct aws_signing_config_aws *get_s3_runtime_signing_config(struct aws_allocator *alloc);
struct aws_credentials_provider *get_s3_runtime_credentials_provider(struct aws_allocator *alloc);

S3Runtime_t *init_s3_runtime();

void destroy_s3_runtime(S3Runtime_t *rt);

#endif /* ___STORAGE_S3_H___ */

/**
 * GOALS:
 * Implement storage_write_stream()
 * Implement storage_read_stream()
 * Translate pull-based AWS semantics ↔ push/pull hybrid storage API
 * Manage AWS runtime lifecycle (event loops, client, creds)
 * Hide multipart upload complexity completely
 */
