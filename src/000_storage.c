#include "include/storage.h"
#include "include/storage_s3.h"
#include "include/storage_ssh.h"
#include "include/storage_sftp.h"


/* -----------------------S3-------------------------------- */
S3State_t *create_s3_state() {
  S3State_t *state = malloc(sizeof(S3State_t));

  if (!state) return NULL;

  state->http_client = NULL;
  state->multipart_upload_id = NULL;

  return NULL;
}

StackStatus_t cleanup_s3_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  S3State_t *state_s3 = (S3State_t *)ctx->state;

  free(state_s3->http_client);
  free(state_s3->multipart_upload_id);

  return status;
}

StorageContext_t *create_s3_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();

  ctx->cleanup = cleanup_s3_state;
  ctx->ops = &storage_ops_table[PTC_S3];
  ctx->state = create_s3_state();

  return ctx;
}
/* ----------------------------------------------------------- */

/* -----------------------SFTP-------------------------------- */
SFTPState_t *create_sftp_state() {
  SFTPState_t *state = malloc(sizeof(SFTPState_t));

  if (!state) return NULL;

  state->open_file_handle = NULL;
  state->ssh_handle = NULL;

  return NULL;
}

StackStatus_t cleanup_sftp_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  SFTPState_t *state_sftp = (SFTPState_t *)ctx->state;

  free(state_sftp->open_file_handle);
  free(state_sftp->ssh_handle);

  return status;
}

StorageContext_t *create_sftp_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();

  ctx->cleanup = cleanup_sftp_state;
  ctx->ops = &storage_ops_table[PTC_S3];
  ctx->state = create_sftp_state();
  return ctx;
}
/* ----------------------------------------------------------- */

/* -----------------------SSH--------------------------------- */
SFTPState_t *create_ssh_state() {
  SSHState_t *state = malloc(sizeof(SSHState_t));

  if (!state) return NULL;

  state->ssh_handle = NULL;

  return NULL;
}

StackStatus_t cleanup_ssh_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  SSHState_t *state_ssh = (SSHState_t *)ctx->state;

  free(state_ssh->ssh_handle);

  return status;
}

StorageContext_t *create_ssh_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();

  ctx->cleanup = cleanup_ssh_state;
  ctx->ops = &storage_ops_table[PTC_S3];
  ctx->state = create_ssh_state();
  return ctx;
}
/* ----------------------------------------------------------- */

StackStatus_t destroy_storage_context(StorageContext_t *ctx) {
  StackStatus_t status = EXEC_SUCCESS;

  ctx->cleanup(ctx);
  free(ctx->state);
  free(ctx);

 return status;
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
