#ifndef ___STORAGE_SFTP_H___
#define ___STORAGE_SFTP_H___

// external library headers

// standard library headers
#include <libssh/sftp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"

// macro defs


typedef struct SFTPState {
  ssh_session    parent_state;  // an sftp state is bound to an ssh session instance
  sftp_session   session;
  sftp_file      file;
  char           tmp_path[BUF_LEN_S];
  char           final_path[BUF_LEN_M];
  ssize_t        byte_transferred;
} SFTPState_t;


StorageContext_t *create_sftp_context();
StorageContext_t *create_ssh_context();


#endif /* ___STORAGE_SFTP_H___ */
