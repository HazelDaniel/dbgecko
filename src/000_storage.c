#include "include/storage.h"


static StorageOps_t storage_ops_table[SUPPORTED_PROTOCOL_COUNT] = {
  [PTC_SSH] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
  [PTC_SFTP] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
  [PTC_S3] = { .write_file = NULL, .mkdir = NULL,
    .read_file = NULL, .delete_file = NULL, .file_exists = NULL},
};

static void *storage_cleanup_table[SUPPORTED_PROTOCOL_COUNT] = {
  [PTC_SSH] = NULL,
  [PTC_SFTP] = NULL,
  [PTC_S3] = NULL,
};

void *create_s3_state() {
  return NULL;
}

StorageContext_t *create_s3_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));

  ctx->cleanup = NULL;
  ctx->ops = &storage_ops_table[PTC_S3];
  ctx->state = create_s3_state();

  return ctx;
}

StorageContext_t *create_sftp_context() {
  StorageContext_t *ctx = NULL;

  return ctx;
}

StorageContext_t *create_ssh_context() {
  StorageContext_t *ctx = NULL;

  return ctx;
}

RemoteStorageProtocol_t extract_protocol_from_uri(const char *uri) {
  RemoteStorageProtocol_t ptc = PTC_UNKNOWN;
  char uri_cp[BUF_LEN_S] = {0};
  const char *token = NULL;

  strcpy(uri_cp, uri);
  token = strtok(uri_cp, ":");

  if (strcmp(token, SSH_PTC_TOKEN) == 0) return PTC_SSH;
  if (strcmp(token, SFTP_PTC_TOKEN) == 0) return PTC_SFTP;
  // TODO: use regex to parse "user@host:..." for ssh
  if (strcmp(token, S3_PTC_TOKEN) == 0) return  PTC_S3;

  return ptc;
}

StorageContext_t *get_storage_context_from_protocol(RemoteStorageProtocol_t ptc) {
  switch (ptc) {
    case PTC_S3: return create_s3_context();
    case PTC_SSH: return create_ssh_context();
    case PTC_SFTP: return create_sftp_context();
    default: return NULL;
  }
}
