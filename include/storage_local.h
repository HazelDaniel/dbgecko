#ifndef ___STORAGE_LOCAL_H___
#define ___STORAGE_LOCAL_H___

// external library headers
// #include <libcurl.h>

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"

// macro defs


typedef struct LocalFSState {
  FILE             *current_file;
  ssize_t          bytes_transferred;
} LocalFSState_t;


LocalFSState_t *create_local_fs_state();

StorageContext_t *create_local_fs_context();


#endif /* ___STORAGE_LOCAL_H___ */
