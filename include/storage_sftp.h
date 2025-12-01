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


typedef struct SFTPState {
  // SSHSessionHandle_t    *ssh_handle;
  void                  *ssh_handle; // TODO: replace once there's ssh header in the unit
  void                  *open_file_handle; // for streaming
} SFTPState_t;


StorageContext_t *create_sftp_context();


#endif /* ___STORAGE_SFTP_H___ */
