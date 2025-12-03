#include "include/storage.h"


StorageStatus_t sftp__write_file (StorageContext_t *ctx, const char *path, const void *data,
  size_t size, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp writing file aiit\n");

  return status;
}

StorageStatus_t sftp__read_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp reading file aiit\n");

  return status;
}

StorageStatus_t sftp__mkdir (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp running mkdir aiit\n");

  return status;
}

StorageStatus_t sftp__delete_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp deleting file aiit\n");

  return status;
}

_Bool sftp__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("sftp file exists aiit\n");

  return status;
}
