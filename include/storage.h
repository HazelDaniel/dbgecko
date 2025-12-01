#ifndef ___STORAGE_H___
#define ___STORAGE_H___

// external library headers

// standard library headers
#include <stdio.h>
#include <stdbool.h>

// internal library headers
#include "globals.h"

// macro defs
#define SSH_PTC_TOKEN ("ssh")
#define SFTP_PTC_TOKEN ("sftp")
#define S3_PTC_TOKEN ("s3")

#define SUPPORTED_PROTOCOL_COUNT (3)


typedef enum {
  STORAGE_OK,
  STORAGE_READ_FAILED,
  STORAGE_MKDIR_FAILED,
  STORAGE_PERM_DENIED,
  STORAGE_WRITE_FAILED,
  STORAGE_CONNECT_FAILED,
  STORAGE_FILE_NOT_FOUND,
  STORAGE_UNKNOWN_ERR
} StorageStatus_t;

typedef enum {
  PTC_SSH,
  PTC_SFTP,
  PTC_S3,
  PTC_UNKNOWN
} RemoteStorageProtocol_t;

typedef  char *StorageErrorMessage_t;

struct StorageContext;
typedef struct StorageContext StorageContext_t;

typedef struct StorageOps {
  StorageStatus_t   (*write_file)(StorageContext_t *ctx, const char *path, const void *data, size_t size, StorageErrorMessage_t *err);
  StorageStatus_t   (*read_file)(StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err);
  StorageStatus_t   (*delete_file)(StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err);
  StorageStatus_t   (*mkdir)(StorageContext_t *ctx, const char *path, StorageErrorMessage_t *err);
  _Bool             (*file_exists)(const StorageContext_t *const ctx, const char *path);
} StorageOps_t;

typedef struct StorageContext {
  StorageOps_t      *ops;
  void              *state; // implementation-specific state (local fs context, cloud credentials, SSH connection, etc.)
  StackStatus_t     (*cleanup)(const StorageContext_t *const ctx);
} StorageContext_t;

RemoteStorageProtocol_t extract_protocol_from_uri(const char *uri);

StorageOps_t *get_storage_ops_table();

StorageContext_t *get_storage_context_from_protocol(RemoteStorageProtocol_t ptc);

StackStatus_t destroy_storage_context(StorageContext_t *ctx);


#endif /* ___STORAGE_H___ */
