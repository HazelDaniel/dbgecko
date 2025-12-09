#ifndef ___STORAGE_H___
#define ___STORAGE_H___

// external library headers
#include <libssh/libssh.h>
#include <libssh/sftp.h>

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"

// macro defs
#define SSH_PTC_TOKEN ("ssh")
#define SFTP_PTC_TOKEN ("sftp")
#define S3_PTC_TOKEN ("s3")

#define SUPPORTED_PROTOCOL_COUNT (5)

#define EMIT_STORAGE_OPS_DEFS(op) \
\
StorageStatus_t op##__delete_file(const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err); \
StorageStatus_t op##__mkdir(const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err); \
StorageStatus_t op##__read_file(const StorageContext_t *ctx, const char *rel_path, StorageDataSource sink, void *local_state, StorageErrorMessage_t *err); \
StorageStatus_t op##__write_open(const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err); \
StorageStatus_t op##__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len, StorageErrorMessage_t *err); \
StorageStatus_t op##__write_close(const StorageContext_t *ctx, const char *tmp_path_override, const char *final_path_override, StorageErrorMessage_t *err); \
StorageStatus_t op##__write_abort(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err); \
\
_Bool op##__file_exists(const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err);


typedef enum {
  STORAGE_OK,
  STORAGE_READ_FAILED,
  STORAGE_MKDIR_FAILED,
  STORAGE_PERM_DENIED,
  STORAGE_WRITE_FAILED,
  STORAGE_CONNECT_FAILED,
  STORAGE_FILE_NOT_FOUND,
  STORAGE_UNKNOWN_ERR,
  STORAGE_NO_SUPPORT,
  STORAGE_DELETE_FAILED,
  STORAGE_ERR_IO
} StorageStatus_t;

typedef  char *StorageErrorMessage_t;

struct StorageContext;
typedef struct StorageContext StorageContext_t;

typedef ssize_t (*StorageDataSource)(
  void              *state,
  void              *buffer,
  size_t            max_size,
  StorageStatus_t   *status_out
);

typedef StorageDataSource StorageDataSink;

/** -------------------------------------------------------------------------------------
 * Overview (stream reads and writes for backup/restore):
 * backup:
 *        DB → dump stream → storage_write_stream → filesystem
 * restore:
 *        file → storage_write_stream → DB restore command stdin
 ----------------------------------------------------------------------------------------
 */

typedef struct StorageOps {
  StorageStatus_t   (*write_open)(const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err);
  StorageStatus_t   (*write_chunk)(const StorageContext_t *ctx, const void *buf, size_t len, StorageErrorMessage_t *err);
  StorageStatus_t   (*write_close)(const StorageContext_t *ctx, const char *tmp_path_override, const char *final_path_override, StorageErrorMessage_t *err);
  StorageStatus_t   (*write_abort)(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err);
  StorageStatus_t   (*read_file)(const StorageContext_t *ctx, const char *src_path, StorageDataSink sink, void *local_state, StorageErrorMessage_t *err);
  StorageStatus_t   (*delete_file)(const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err);
  StorageStatus_t   (*mkdir)(const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err);
  _Bool             (*file_exists)(const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err);
} StorageOps_t;

typedef struct StorageContext {
  StorageOps_t      *ops;
  void              *state; // implementation-specific state (local fs context, cloud credentials, SSH connection, etc.)
  StackStatus_t     (*init)(const StorageContext_t *const ctx);
  StackStatus_t     (*cleanup)(const StorageContext_t *const ctx);
} StorageContext_t;


RemoteStorageProtocol_t extract_protocol_from_uri(const char *uri);
RemoteStorageProtocol_t ___unsafe_to_ptc___(size_t obj);

StorageOps_t *get_storage_ops_table();

StorageContext_t *get_storage_context_from_protocol(RemoteStorageProtocol_t ptc);

StorageStatus_t storage_write_stream(StorageContext_t *ctx, const char *path, StorageDataSource source,
  void *userdata, StorageErrorMessage_t *err);

StorageStatus_t storage_read_stream(StorageContext_t *ctx, const char *src_path, StorageDataSink sink,
  void *local_state, StorageErrorMessage_t *err);

StackStatus_t destroy_storage_context(StorageContext_t *ctx);

EMIT_STORAGE_OPS_DEFS(unknown)
EMIT_STORAGE_OPS_DEFS(s3)
EMIT_STORAGE_OPS_DEFS(sftp)
EMIT_STORAGE_OPS_DEFS(local_fs)
EMIT_STORAGE_OPS_DEFS(ssh)


#endif /* ___STORAGE_H___ */
