#ifndef ___STORAGE_S3_H___
#define ___STORAGE_S3_H___

// external library headers
// #include <libcurl.h>

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"
#include "storage.h"

// macro defs


typedef struct S3State {
  // runtime metadata
  // CURL *http_client;
  void             *http_client; // TODO: change to CURL pointer ones libcurl has finished installing
  char             *multipart_upload_id;
} S3State_t;


S3State_t *create_s3_state();

StorageContext_t *create_s3_context();


#endif /* ___STORAGE_S3_H___ */
