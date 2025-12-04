#ifndef ___STORAGE_SSH_H___
#define ___STORAGE_SSH_H___

// external library headers

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"

// macro defs


typedef struct SSHState {
  size_t         max_retries;
  size_t         timeout_seconds;
  char           username[BUF_LEN_S];
  char           host[BUF_LEN_XS];
  char           private_key[BUF_LEN_S];
  _Bool          verify_known_hosts;
  size_t         port;
  ssh_session    session;
} SSHState_t;


SSHState_t *create_ssh_state(const char *private_key, const char *host, const char *username, size_t port,
  size_t max_retries, size_t timeout_seconds, _Bool verify_known_hosts);

StorageContext_t *create_ssh_context();


#endif /* ___STORAGE_SSH_H___ */
