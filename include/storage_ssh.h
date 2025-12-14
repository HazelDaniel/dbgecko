#ifndef ___STORAGE_SSH_H___
#define ___STORAGE_SSH_H___

// external library headers

// standard library headers
#include <libssh/libssh.h>
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


SSHState_t *create_ssh_state(_Bool verify_known_hosts);

StorageContext_t *create_ssh_context();


#endif /* ___STORAGE_SSH_H___ */
