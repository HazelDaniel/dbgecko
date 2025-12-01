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
  // SSHSessionHandle_t    *ssh_handle;
  void                  *ssh_handle; // TODO: replace once there's ssh header in the unit
} SSHState_t;


StorageContext_t *create_ssh_context();


#endif /* ___STORAGE_SSH_H___ */
