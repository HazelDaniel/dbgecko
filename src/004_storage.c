#include "include/storage.h"


StorageStatus_t sftp__write_close(const StorageContext_t *ctx, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp writing file close aiit\n");

  return status;
}

StorageStatus_t sftp__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp writing file chunk aiit\n");

  return status;
}

StorageStatus_t sftp__write_open(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp writing file open aiit\n");

  return status;
}

StorageStatus_t sftp__write_abort(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp writing file abort aiit\n");

  return status;
}

StorageStatus_t sftp__read_file (const StorageContext_t *ctx, const char *rel_path,
  StorageDataSource sink, void *userdata, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp reading file aiit\n");

  return status;
}

StorageStatus_t sftp__mkdir (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp running mkdir aiit\n");

  return status;
}

StorageStatus_t sftp__delete_file (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp deleting file aiit\n");

  return status;
}

_Bool sftp__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp file exists aiit\n");

  return status;
}
