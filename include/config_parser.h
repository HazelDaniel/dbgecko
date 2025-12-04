#ifndef ___CONFIG_PARSER_H___
#define ___CONFIG_PARSER_H___

// external library headers
#include <stddef.h>
#include <stdint.h>
#include <yaml.h>

// standard library headers
#include <stdbool.h>

//internal library headers
#include "globals.h"

//macro defs
#define DB_TYPE_STR_PG ("postgres")
#define DB_TYPE_STR_MYSQL ("mysql")
#define DB_TYPE_STR_MONGO ("mongodb")

#define DEFAULT_PLATFORM_VERSION (1.0)
#define DEFAULT_PLUGIN_DIR_PATH ("")
#define DEFAULT_PLUGIN_PATH ("") /* leave empty */

#define DEFAULT_DB_URI ("default:uri")
#define DEFAULT_DB_TYPE ("default:type")
#define DEFAULT_DB_BACKUP_MODE ("default:backup_mode")
#define DEFAULT_DB_TIMEOUT (1000)

#define DEFAULT_STORAGE_OUTPUT_PATH ("default:output_path")
#define DEFAULT_STORAGE_COMPRESSION ("default:compression")
#define DEFAULT_STORAGE_ENC_KEY_PATH ("default:encryption_key_path")
#define DEFAULT_STORAGE_REMOTE ("default:remote")

#define DEFAULT_RUNTIME_LOG_LEVEL (1)
#define DEFAULT_RUNTIME_THREAD_COUNT (1)
#define DEFAULT_RUNTIME_TMP_DIR ("default:tmp_dir")


typedef struct DBConfig {
  char             type[BUF_LEN_XS];
  char             backup_mode[BUF_LEN_XS];
  char             uri[BUF_LEN_S];
  _Bool            online;
  size_t           timeout_seconds;
} DBConfig_t;

typedef enum {
  SECTION_NONE,
  SECTION_DB,
  SECTION_STORAGE,
  SECTION_RUNTIME,
  SECTION_PLATFORM,
  SECTION_PLUGIN,
  SECTION_CHILD_S3,
  SECTION_CHILD_SSH,
  SECTION_CHILD_SFTP
} config_section_t;

typedef enum {
  EXPECT_KEY,
  EXPECT_VALUE
} parse_phase_t;

typedef struct S3Config {
  char           region[BUF_LEN_XS];
  char           endpoint[BUF_LEN_S];
  char           access_key[BUF_LEN_S];
  char           secret_key[BUF_LEN_M];
  char           path_style[BUF_LEN_XS];
  char           session_token[BUF_LEN_S];
  size_t         max_retries;
  size_t         timeout_seconds;
  size_t         multipart_threshold_mb;
  size_t         multipart_chunk_mb;
  _Bool          use_ssl;
} S3Config_t;

typedef struct SSHConfig {
  size_t         max_retries;
  size_t         timeout_seconds;
  char           username[BUF_LEN_S];
  char           host[BUF_LEN_XS];
  char           private_key[BUF_LEN_S];
  _Bool          verify_known_hosts;
  size_t         port;
} SSHConfig_t;

typedef struct SFTPConfig {
  char           private_key[BUF_LEN_S];
  char           username[BUF_LEN_S];
  char           host[BUF_LEN_XS];
  size_t         port;
  size_t         max_retries;
  size_t         timeout_seconds;
} SFTPConfig_t;

typedef struct {
  RemoteStorageProtocol_t         kind;
  union {
    S3Config_t            s3;
    SSHConfig_t           ssh;
    SFTPConfig_t          sftp;
  }                               backend;
} StorageBackendConfig_t;

typedef struct storageConfig {
  char                      output_path[BUF_LEN_S];
  char                      compression[BUF_LEN_XS];
  char                      encryption_key_path[BUF_LEN_S];
  char                      remote_target[BUF_LEN_S];
  StorageBackendConfig_t    *backend;
} StorageConfig_t;

typedef struct RuntimeConfig {
  size_t          log_level;
  size_t          thread_count;
  char            temp_dir[BUF_LEN_S];
} RuntimeConfig_t;

typedef struct PlatformConfig {
  float           version;
} PlatformConfig_t;

typedef struct PluginConfig {
  char            dir[BUF_LEN_S];
  char            path[BUF_LEN_M];
} PluginConfig_t;

typedef struct AppConfig {
  DBConfig_t           *db;
  StorageConfig_t      *storage;
  RuntimeConfig_t      *runtime;
  PlatformConfig_t     *platform;
  PluginConfig_t       *plugin;
} AppConfig_t;

typedef enum {
  CONFIG_OK = 0, //order important for casting into stack error
  CONFIG_FILE_NOT_FOUND,
  CONFIG_PARSE_ERROR,
  CONFIG_VALIDATION_ERROR,
  CONFIG_MEMORY_ERROR
} ConfigParserStatus_t;

typedef struct ConfigParserError {
  ConfigParserStatus_t        code;
  char                        message[BUF_LEN_M];
  size_t                      line;
  size_t                      column;
} ConfigParserError_t;


/**
 * config_load_file - loads config file
 * @path: location of config file
 * @out_config: the output config options in a defined struct
 * @err: written error object on failure
 *
 * Return: ConfigParserStatus_t
 * ~NOTE~:
 **/
ConfigParserStatus_t config_load_file(const char *path, AppConfig_t *out_config, ConfigParserError_t **err);

DBConfig_t *init_db_config(const char *type, const char *uri, const char *backup_mode, size_t timeout_seconds, _Bool online);

StorageConfig_t *init_storage_config(const char *output_path, const char *compression, const char *encryption_key_path, const char *remote_target);

RuntimeConfig_t *init_runtime_config(size_t log_level, size_t thread_count, const char *temp_dir);

PlatformConfig_t *init_platform_config(float version);

PluginConfig_t *init_plugin_config(const char *dir, const char *path);

AppConfig_t *init_app_config(DBConfig_t *db, StorageConfig_t *storage, RuntimeConfig_t *runtime,
  PlatformConfig_t *platform, PluginConfig_t *plugin);
AppConfig_t **get_app_config_handle();

ConfigParserError_t *create_parser_error();

int assign_yaml_parsed_value(config_section_t section, const char *key,
  const char *value, AppConfig_t *cfg, ConfigParserError_t *err);

void print_app_config(AppConfig_t *cfg);
void validate_app_config(AppConfig_t *cfg, StackError_t **err);
void destroy_app_config();
void destroy_parser_error(ConfigParserError_t **err);
/**
 * merge_configs - merges cli argument configs with the yaml configs
 *  to both override the default configs. then sets the global app_config
 * @argc: argument count from main
 * @argv: argument vector from main
 * @err: written stack error object on failure
 *
 * Return: void
 * ~NOTE~:
 **/
void merge_configs(int argc, char **argv, StackError_t **err);
void set_app_config(AppConfig_t *cfg);

char *get_storage_protocol_text(RemoteStorageProtocol_t ptc);


#endif /* ___CONFIG_PARSER_H___ */
