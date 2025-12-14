#include "include/storage.h"


StorageStatus_t ssh__write_close(const StorageContext_t *ctx, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh writing file close aiit\n");

  return status;
}

StorageStatus_t ssh__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh writing file chunk aiit\n");

  return status;
}

StorageStatus_t ssh__write_open(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh writing file open aiit\n");

  return status;
}

StorageStatus_t ssh__write_abort(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh writing file abort aiit\n");

  return status;
}

StorageStatus_t ssh__read_file (const StorageContext_t *ctx, const char *rel_path, StorageDataSource sink,
  void *userdata, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh reading file aiit\n");

  return status;
}

StorageStatus_t ssh__mkdir (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh running mkdir aiit\n");

  return status;
}

StorageStatus_t ssh__delete_file (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh deleting file aiit\n");

  return status;
}

_Bool ssh__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("ssh file exists aiit\n");

  return status;
}
