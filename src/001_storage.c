#include "include/storage.h"


static StorageOps_t storage_ops_table[SUPPORTED_PROTOCOL_COUNT] = {
  [PTC_SSH] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
  [PTC_SFTP] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
  [PTC_S3] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
};

StorageOps_t *get_storage_ops_table() {
  return storage_ops_table;
}
