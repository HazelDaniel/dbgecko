#include "include/storage.h"
#include "include/config_parser.h"
#include "include/storage_s3.h"
#include "include/storage_ssh.h"
#include "include/storage_sftp.h"


StackStatus_t destroy_local_ssh_state(SSHState_t *state_ssh) {
  StackStatus_t status = EXEC_SUCCESS;

  if (state_ssh->session) ssh_free(state_ssh->session);

  return status;
}

/* -----------------------S3-------------------------------- */
S3State_t *create_s3_state() {
  S3State_t *state = malloc(sizeof(S3State_t));

  if (!state) return NULL;

  state->http_client = NULL;
  state->multipart_upload_id = NULL;

  return state;
}

StackStatus_t cleanup_s3_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  S3State_t *state_s3 = (S3State_t *)ctx->state;

  if (state_s3->http_client) free(state_s3->http_client);
  if (state_s3->multipart_upload_id) free(state_s3->multipart_upload_id);

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
  AppConfig_t *cfg = *get_app_config_handle();
  SFTPConfig_t sftp;

  if (!state) return NULL;
  if (!cfg) return NULL;

  sftp = cfg->storage->backend->backend.sftp;

  state->parent_state = create_ssh_state(state->private_key, state->username, state->port,
    state->max_retries, state->timeout_seconds, false);

  if (!state->parent_state) return NULL;

  state->max_retries = sftp.max_retries;
  state->port = sftp.port;
  state->timeout_seconds = sftp.timeout_seconds;
  snprintf(state->username, BUF_LEN_S, "%s", sftp.username);
  snprintf(state->private_key, BUF_LEN_S, "%s", sftp.private_key);
  state->session = sftp_new(state->parent_state->session);

  return state;
}

StackStatus_t cleanup_sftp_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  SFTPState_t *state_sftp = (SFTPState_t *)ctx->state;

  if (state_sftp->parent_state) {
    status = destroy_local_ssh_state(state_sftp->parent_state);

    if (status != EXEC_SUCCESS) {

      sftp_free(state_sftp->session);
      return status;
    }
  }

  sftp_free(state_sftp->session);

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
SSHState_t *create_ssh_state(const char *private_key, const char *username, size_t port,
  size_t max_retries, size_t timeout_seconds, bool verify_known_hosts) {
  SSHState_t *state = malloc(sizeof(SSHState_t));

  if (!state) return NULL;

  state->max_retries = max_retries;
  state->port = port;
  state->verify_known_hosts = verify_known_hosts;
  state->timeout_seconds = timeout_seconds;
  snprintf(state->username, BUF_LEN_S, "%s", username);
  snprintf(state->private_key, BUF_LEN_S, "%s", private_key);
  state->session = ssh_new();

  return state;
}

StackStatus_t cleanup_ssh_state (const StorageContext_t *const ctx) {
  StackStatus_t status = EXEC_SUCCESS;
  SSHState_t *state_ssh = (SSHState_t *)(ctx->state);

  if (!state_ssh) return EXEC_FAILURE;

  status = destroy_local_ssh_state(state_ssh);

  return status;
}

StorageContext_t *create_ssh_context() {
  StorageContext_t *ctx = malloc(sizeof(StorageContext_t));
  StorageOps_t *storage_ops_table = get_storage_ops_table();
  AppConfig_t *cfg = *get_app_config_handle();
  SSHConfig_t ssh;

  if (!cfg) return NULL;

  ssh = cfg->storage->backend->backend.ssh;

  ctx->cleanup = cleanup_ssh_state;
  ctx->ops = &storage_ops_table[PTC_SSH];
  ctx->state = create_ssh_state(ssh.private_key, ssh.username, ssh.port, ssh.max_retries, ssh.timeout_seconds, ssh.verify_known_hosts);

  if (!ctx->state) {
    ctx->cleanup = NULL, ctx->ops = NULL, ctx->state = NULL;
    free(ctx);

    return NULL;
  }

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

RemoteStorageProtocol_t ___unsafe_to_ptc___(size_t obj) {
  return (RemoteStorageProtocol_t)obj;
}
