#ifndef ___STORAGE_LOCAL_H___
#define ___STORAGE_LOCAL_H___

// external library headers

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"

// macro defs


typedef struct LocalFSState {
  ssize_t          bytes_transferred;
  char             tmp_path[BUF_LEN_S];
  char             final_path[BUF_LEN_M];
  int              fd;
} LocalFSState_t;

typedef struct LocalFSStreamReadState {
  FILE             *fp;
  size_t           total_size;
  size_t           cursor;
} LocalFSStreamReadState_t;


LocalFSState_t *create_local_fs_state();

StorageContext_t *create_local_fs_context();


#endif /* ___STORAGE_LOCAL_H___ */
