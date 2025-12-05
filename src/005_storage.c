#include "include/storage.h"


StorageStatus_t local_fs__write_file (StorageContext_t *ctx, const char *path, const void *data,
  size_t size, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("local_fs writing file aiit\n");

  return status;
}

StorageStatus_t local_fs__read_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("local_fs reading file aiit\n");

  return status;
}

StorageStatus_t local_fs__mkdir (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("local_fs running mkdir aiit\n");

  return status;
}

StorageStatus_t local_fs__delete_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("local_fs deleting file aiit\n");

  return status;
}

_Bool local_fs__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("local_fs file exists aiit\n");

  return status;
}
