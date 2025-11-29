#ifndef ___STORAGE_S3_H___
#define ___STORAGE_S3_H___

// external library headers

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"

// macro defs


typedef struct S3State {
  char        *endpoint;
  char        *bucket;
  char        *access_key;
  char        *secret_key;
  char        *session_token;
  _Bool       ssl_on;
} S3State_t;


#endif /* ___STORAGE_S3_H___ */
