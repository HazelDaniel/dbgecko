#ifndef ___GLOBALS_H___
#define ___GLOBALS_H___

// external libraries
#include "uthash.h"

#define CURRENT_PLATFORM_VERSION (1.0)
#define VERSION_SUPPORT_RANGE (0.5)

// utf-8 aware regex parsing for pcre library
#define PCRE2_CODE_UNIT_WIDTH 8

#define BUF_LEN_XS (64)
#define BUF_LEN_SS (128)
#define BUF_LEN_S (256)
#define BUF_LEN_M (512)
#define BUF_LEN (1024)
#define BUF_LEN_L (4096)

#define CFG_STORAGE_PREFIX(x) ("storage_"#x)
#define CFG_PLATFORM_PREFIX(x) ("platform_"#x)
#define CFG_PLUGIN_PREFIX(x) ("plugin_"#x)
#define CFG_DB_PREFIX(x) ("db_"#x)
#define CFG_RUNTIME_PREFIX(x) ("runtime_"#x)
#define CFG_PATH ("config_path")

typedef enum {
  EXEC_SUCCESS = 0,
  EXEC_FAILURE = -1,
} StackStatus_t;

typedef enum { // DO NOT CHANGE ORDER. will be used in conversions
  PTC_UNKNOWN,
  PTC_S3,
  PTC_SFTP,
  PTC_LOCAL
} RemoteStorageProtocol_t;

typedef enum {
  DB_TYPE_UNKNOWN = 0,
  DB_TYPE_MONGO,
  DB_TYPE_POSTGRES,
  DB_TYPE_MYSQL
} DBType_t;

typedef struct StackError {
  StackStatus_t        code;
  char                 message[BUF_LEN_M];
} StackError_t;

typedef char *StackErrorMessage_t;
typedef char OutputBuffer_t[BUF_LEN_L];


StackError_t *create_stack_error();
StackError_t *___unsafe_to_stack_error___(void *err);

int join_path(const char *base, const char *rel, char *out, size_t outlen);

void set_err(const char **err, size_t size, const char *fmt, ...);
void destroy_stack_error(StackError_t *err);


#endif /* ___GLOBALS_H___ */
