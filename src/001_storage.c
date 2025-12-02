#include "include/storage.h"


#define UNKNOWN_IMPL_BODY(name,err) \
{ \
  *err = malloc(BUF_LEN_XS * sizeof(char)); \
  snprintf(*err, BUF_LEN_XS - 1, "cannot implement '" #name "' for unknown protocol!"); \
\
  return STORAGE_NO_SUPPORT; \
}

StorageStatus_t unknown_write_file (StorageContext_t *ctx, const char *path, const void *data,
  size_t size, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(write_file, err);
}

StorageStatus_t unknown_read_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(read_file, err);
}

StorageStatus_t unknown_mkdir (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(mkdir, err);
}

StorageStatus_t unknown_delete_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(delete_file, err);
}

_Bool unknown_file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  UNKNOWN_IMPL_BODY(file_exists, err);
}

static StorageOps_t storage_ops_table[SUPPORTED_PROTOCOL_COUNT] = {
  [PTC_SSH] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
  [PTC_SFTP] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
  [PTC_S3] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
  [PTC_UNKNOWN] = {.write_file = unknown_write_file, .mkdir = unknown_mkdir,
    .read_file = unknown_read_file, .delete_file = unknown_delete_file,
    .file_exists = unknown_file_exists}
};

StorageOps_t *get_storage_ops_table() {
  return storage_ops_table;
}
