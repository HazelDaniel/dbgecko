#include "include/storage.h"


#define UNKNOWN_IMPL_BODY(name,err) \
{ \
  *err = malloc(BUF_LEN_XS * sizeof(char)); \
  snprintf(*err, BUF_LEN_XS - 1, "cannot implement '" #name "' for unknown protocol!"); \
\
  return STORAGE_NO_SUPPORT; \
}

static StorageOps_t storage_ops_table[SUPPORTED_PROTOCOL_COUNT] = {
  [PTC_SSH] = { .write_file = ssh__write_file, .mkdir = ssh__mkdir,
    .read_file = ssh__read_file, .delete_file = ssh__delete_file, .file_exists = ssh__file_exists },
  [PTC_SFTP] = { .write_file = sftp__write_file, .mkdir = sftp__mkdir,
    .read_file = sftp__read_file, .delete_file = sftp__delete_file, .file_exists = sftp__file_exists },
  [PTC_S3] = { .write_file = s3__write_file, .mkdir = s3__mkdir,
    .read_file = s3__read_file, .delete_file = s3__delete_file, .file_exists = s3__file_exists },
  [PTC_LOCAL] = { .write_file = local_fs__write_file, .mkdir = local_fs__mkdir,
    .read_file = local_fs__read_file, .delete_file = local_fs__delete_file, .file_exists = local_fs__file_exists },
  [PTC_UNKNOWN] = { .write_file = unknown__write_file, .mkdir = unknown__mkdir,
    .read_file = unknown__read_file, .delete_file = unknown__delete_file,
    .file_exists = unknown__file_exists }
};

StorageOps_t *get_storage_ops_table() {
  return storage_ops_table;
}

StorageStatus_t unknown__write_file (StorageContext_t *ctx, const char *path, const void *data,
  size_t size, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(write_file, err);
}

StorageStatus_t unknown__read_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(read_file, err);
}

StorageStatus_t unknown__mkdir (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(mkdir, err);
}

StorageStatus_t unknown__delete_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(delete_file, err);
}

_Bool unknown__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(file_exists, err);
}
