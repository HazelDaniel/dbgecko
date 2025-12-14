#include "include/storage.h"


StorageStatus_t s3__write_open(const StorageContext_t *ctx, const char *rel_path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 writing file open aiit\n");

  return status;
}

StorageStatus_t s3__write_chunk(const StorageContext_t *ctx, const void *buf, size_t len, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 writing file chunks aiit\n");

  return status;
}

StorageStatus_t s3__write_close(const StorageContext_t *ctx, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 writing file close aiit\n");

  return status;
}

StorageStatus_t s3__write_abort(const StorageContext_t *ctx, const char *tmp_path_override, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 writing file abort aiit\n");

  return status;
}

StorageStatus_t s3__read_file (const StorageContext_t *ctx, const char *rel_path, StorageDataSource sink,
  void *userdata, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 reading file aiit\n");

  return status;
}

StorageStatus_t s3__mkdir (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 running mkdir aiit\n");

  return status;
}

StorageStatus_t s3__delete_file (const StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 deleting file aiit\n");

  return status;
}

_Bool s3__file_exists (const StorageContext_t *const ctx, const char *path, StorageErrorMessage_t *err) {
  StorageStatus_t status = STORAGE_OK;

  printf("s3 file exists aiit\n");

  return status;
}
