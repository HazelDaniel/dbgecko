#ifndef ___STORAGE_SFTP_H___
#define ___STORAGE_SFTP_H___

// external library headers

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"

// macro defs


typedef struct SSHState {
  sftp_file      file;
  char           tmp_path[BUF_LEN_S];
  char           final_path[BUF_LEN_M];
  size_t         bytes_transferred;
  ssh_session    session;
} SSHState_t;

typedef struct SFTPState {
  SSHState_t     *parent_state;  // an sftp state is bound to an ssh session instance
  sftp_session   session;
  char           tmp_path[BUF_LEN_S];
  char           final_path[BUF_LEN_M];
} SFTPState_t;


StorageContext_t *create_sftp_context();
StorageContext_t *create_ssh_context();

SSHState_t *create_ssh_state(_Bool verify_known_hosts);


#endif /* ___STORAGE_SFTP_H___ */
