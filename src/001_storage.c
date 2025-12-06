#include "include/storage.h"


#define UNKNOWN_IMPL_BODY(name,err) \
{ \
  *err = malloc(BUF_LEN_XS * sizeof(char)); \
  snprintf(*err, BUF_LEN_XS - 1, "cannot implement '" #name "' for unknown protocol!"); \
\
  return STORAGE_NO_SUPPORT; \
}

static StorageOps_t storage_ops_table[SUPPORTED_PROTOCOL_COUNT] = {
  [PTC_SSH] = { .write_abort = ssh__write_abort, .write_chunk = ssh__write_chunk, .write_close = ssh__write_close,
    .mkdir = ssh__mkdir, .read_file = ssh__read_file, .delete_file = ssh__delete_file, .file_exists = ssh__file_exists },
  [PTC_SFTP] = { .write_abort = sftp__write_abort, .write_chunk = sftp__write_chunk, .write_close = sftp__write_close,
    .mkdir = sftp__mkdir, .read_file = sftp__read_file, .delete_file = sftp__delete_file, .file_exists = sftp__file_exists },
  [PTC_S3] = { .write_abort = s3__write_abort, .write_chunk = s3__write_chunk, .write_close = s3__write_close,
    .mkdir = s3__mkdir, .read_file = s3__read_file, .delete_file = s3__delete_file, .file_exists = s3__file_exists },
  [PTC_LOCAL] = { .write_abort = local_fs__write_abort, .write_chunk = local_fs__write_chunk, .write_close = local_fs__write_close,
    .mkdir = local_fs__mkdir, .read_file = local_fs__read_file, .delete_file = local_fs__delete_file, .file_exists = local_fs__file_exists },
  [PTC_UNKNOWN] = { .write_abort = unknown__write_abort, .write_chunk = unknown__write_chunk, .write_close = unknown__write_close,
    .mkdir = unknown__mkdir, .read_file = unknown__read_file, .delete_file = unknown__delete_file, .file_exists = unknown__file_exists }
};

StorageOps_t *get_storage_ops_table() {
  return storage_ops_table;
}

StorageStatus_t unknown__write_abort(const StorageContext_t *ctx, const char *tmp_path_override,
  StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(write_abort, err);
}

StorageStatus_t unknown__write_close(const StorageContext_t *ctx, const char *tmp_path_override,
  const char *final_path_override, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(write_close, err);
}

StorageStatus_t unknown__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len,
  StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(write_chunk, err);
}

StorageStatus_t unknown__write_open(const StorageContext_t *ctx, const char *rel_path,
  StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(write_open, err);
}

StorageStatus_t unknown__read_file (const StorageContext_t *ctx, const char *rel_path, StorageDataSource sink,
  void *userdata, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(read_file, err);
}

StorageStatus_t unknown__mkdir (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(mkdir, err);
}

StorageStatus_t unknown__delete_file (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(delete_file, err);
}

_Bool unknown__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(file_exists, err);
}
