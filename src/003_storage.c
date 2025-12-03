#include "include/storage.h"


StorageStatus_t ssh__write_file (StorageContext_t *ctx, const char *path, const void *data,
  size_t size, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh writing file aiit\n");

  return status;
}

StorageStatus_t ssh__read_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh reading file aiit\n");

  return status;
}

StorageStatus_t ssh__mkdir (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh running mkdir aiit\n");

  return status;
}

StorageStatus_t ssh__delete_file (StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh deleting file aiit\n");

  return status;
}

_Bool ssh__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh file exists aiit\n");

  return status;
}
