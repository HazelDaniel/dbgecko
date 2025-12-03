#include "include/storage.h"

StorageStatus_t s3__write_file (StorageContext_t *ctx, const char *path, const void *data,
  size_t size, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 writing file aiit\n");

  return status;
}

StorageStatus_t s3__read_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 reading file aiit\n");

  return status;
}

StorageStatus_t s3__mkdir (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 running mkdir aiit\n");

  return status;
}

StorageStatus_t s3__delete_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 deleting file aiit\n");

  return status;
}

_Bool s3__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 file exists aiit\n");

  return status;
}
