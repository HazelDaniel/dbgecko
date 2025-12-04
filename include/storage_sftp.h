#ifndef ___STORAGE_SFTP_H___
#define ___STORAGE_SFTP_H___

// external library headers

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"
#include "storage_ssh.h"

// macro defs


typedef struct SFTPState {
  char           private_key[BUF_LEN_S];
  char           username[BUF_LEN_S];
  char           host[BUF_LEN_XS];
  size_t         port;
  size_t         max_retries;
  size_t         timeout_seconds;

  SSHState_t     *parent_state;  // an sftp state is bound to an ssh session instance
  sftp_session   session;
} SFTPState_t;


StorageContext_t *create_sftp_context();


#endif /* ___STORAGE_SFTP_H___ */
