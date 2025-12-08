#include "include/config_parser.h"
#include <pcre2.h>
#include <string.h>


static AppConfig_t *app_config = NULL;

AppConfig_t **get_app_config_handle() {
  return &app_config;
}

void set_app_config(AppConfig_t *cfg) {
  AppConfig_t **cfg_ptr = get_app_config_handle();

  if (!cfg_ptr) return;
  if (*cfg_ptr) destroy_app_config();
  *cfg_ptr = cfg;
}

DBConfig_t *init_db_config(const char *type, const char *uri, const char *backup_mode, size_t timeout_seconds, _Bool online) {
  DBConfig_t *cfg = malloc(sizeof(DBConfig_t));

  if (!cfg) return NULL;
  strncpy(cfg->type, type, sizeof(cfg->type) - 1);
  cfg->type[sizeof(cfg->type) - 1] = '\0';
  strncpy(cfg->uri, uri, sizeof(cfg->uri) - 1);
  strncpy(cfg->backup_mode, backup_mode, sizeof(cfg->backup_mode) - 1);
  cfg->uri[sizeof(cfg->uri) - 1] = '\0';
  cfg->timeout_seconds = timeout_seconds;
  cfg->online = false;

  return cfg;
}

StorageConfig_t *init_storage_config(const char *output_path, const char *compression, const char *encryption_key_path, const char *remote_target) {
  StorageConfig_t *cfg = malloc(sizeof(StorageConfig_t));

  if (!cfg) return NULL;
  cfg->backend = malloc(sizeof(StorageBackendConfig_t));
  if (!cfg->backend) return NULL;

  strncpy(cfg->output_path, output_path, sizeof(cfg->output_path) - 1);
  cfg->output_path[sizeof(cfg->output_path) - 1] = '\0';
  strncpy(cfg->compression, compression, sizeof(cfg->compression) - 1);
  cfg->compression[sizeof(cfg->compression) - 1] = '\0';
  strncpy(cfg->encryption_key_path, encryption_key_path, sizeof(cfg->encryption_key_path) - 1);
  cfg->encryption_key_path[sizeof(cfg->encryption_key_path) - 1] = '\0';
  strncpy(cfg->remote_target, remote_target, sizeof(cfg->remote_target) - 1);
  cfg->remote_target[sizeof(cfg->remote_target) - 1] = '\0';


  return cfg;
}

RuntimeConfig_t *init_runtime_config(size_t log_level, size_t thread_count, const char *temp_dir) {
  RuntimeConfig_t *cfg = malloc(sizeof(RuntimeConfig_t));

  if (!cfg) return NULL;
  cfg->log_level = log_level;
  cfg->thread_count = thread_count;
  strncpy(cfg->temp_dir, temp_dir, sizeof(cfg->temp_dir) - 1);
  cfg->temp_dir[sizeof(cfg->temp_dir) - 1] = '\0';

  return cfg;
}

PlatformConfig_t *init_platform_config(float version) {
  PlatformConfig_t *cfg = malloc(sizeof(PlatformConfig_t));

  if (!cfg) return NULL;
  cfg->version = version;

  return cfg;
}

PluginConfig_t *init_plugin_config(const char *dir, const char *path) {
  PluginConfig_t *cfg = malloc(sizeof(PluginConfig_t));

  if (!cfg) return NULL;
  strncpy(cfg->dir, dir, sizeof(cfg->dir) - 1);
  cfg->dir[sizeof(cfg->dir) - 1] = '\0';

  if (!path) return cfg;
  strncpy(cfg->path, path, sizeof(cfg->path) - 1);
  cfg->path[sizeof(cfg->path) - 1] = '\0';

  return cfg;
}

AppConfig_t *init_app_config(DBConfig_t *db, StorageConfig_t *storage, RuntimeConfig_t *runtime,
  PlatformConfig_t *platform, PluginConfig_t *plugin) {
  AppConfig_t *cfg = malloc(sizeof(AppConfig_t));

  if (!cfg) {
    if (db) free(db);
    if (storage) free(storage);
    if (runtime) free(runtime);
    if (platform) free(platform);
    if (plugin) free(plugin);

    return NULL;
  }

  cfg->db = db;
  cfg->storage = storage;
  cfg->runtime = runtime;
  cfg->platform = platform;
  cfg->plugin = plugin;

  return cfg;
}

ConfigParserError_t *create_parser_error() {
  ConfigParserError_t *err = malloc(sizeof(ConfigParserError_t));
  err->code = CONFIG_OK;
  err->line = 0;
  err->column = 0;

  return err;
}

void destroy_app_config() {
  AppConfig_t *app_cfg = NULL, **app_cfg_ref = get_app_config_handle();

  app_cfg = *app_cfg_ref;

  if (!app_cfg) {
    return;
  }

  if (app_cfg->db) free(app_cfg->db);

  if (app_cfg->storage) {
    if (app_cfg->storage->backend) free(app_cfg->storage->backend);
    free(app_cfg->storage);
  }

  if (app_cfg->runtime) free(app_cfg->runtime);
  if (app_cfg->platform) free(app_cfg->platform);
  if (app_cfg->plugin) free(app_cfg->plugin);

  free(app_cfg);
  *app_cfg_ref = NULL;
}

void destroy_parser_error(ConfigParserError_t **err) {
  if (!err) return;
  if (!*err) {
    free(err);

    return;
  }

  free(*err);
  *err = NULL;
}

void validate_app_config(AppConfig_t *cfg, StackError_t **err) {
  #define VALIDATE_COND_WITH_MESSAGE(cond, message) \
  {\
    if (cond) { \
      *err = create_stack_error(); \
      strcpy((*err)->message, message); \
      if (cfg) destroy_app_config(); \
      return; \
    } \
    \
  }

  char *message = NULL;

  message = "config doesn't exist!";
  VALIDATE_COND_WITH_MESSAGE((!cfg), message);

  message = "invalid platform version!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->platform->version), message);

  message = "invalid timeout seconds value for db!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->db->timeout_seconds), message);

  message = "backup mode invalid!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->db->backup_mode[0] || (strcmp(cfg->db->backup_mode, "incremental") != 0 &&\
    strcmp(cfg->db->backup_mode, "full") != 0 && strcmp(cfg->db->backup_mode, "schema-only") != 0)), message);

  message = "invalid db type!";
  VALIDATE_COND_WITH_MESSAGE((strcmp(cfg->db->type, DB_TYPE_STR_PG) != 0 &&\
    strcmp(cfg->db->type, DB_TYPE_STR_MYSQL) != 0 && strcmp(cfg->db->type, DB_TYPE_STR_MONGO) != 0), message);

  message = "uri not provided!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->db->uri[0]), message);

  message = "output path not provided!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->storage->output_path[0]), message);

  message = "encryption key path not provided!";
  VALIDATE_COND_WITH_MESSAGE((cfg->db->online && !cfg->storage->encryption_key_path[0]), message);

  message = "remote location not provided!";
  VALIDATE_COND_WITH_MESSAGE((cfg->db->online && !cfg->storage->remote_target[0]), message);

  message = "invalid log level!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->runtime->log_level), message);

  message = "invalid thread count!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->runtime->thread_count), message);

  message = "temporary runtime directory not provided!";
  VALIDATE_COND_WITH_MESSAGE((!cfg->runtime->temp_dir[0]), message);

  message = "incompatible platform version";
  VALIDATE_COND_WITH_MESSAGE((cfg->platform->version < CURRENT_PLATFORM_VERSION - VERSION_SUPPORT_RANGE ||
    cfg->platform->version > CURRENT_PLATFORM_VERSION + VERSION_SUPPORT_RANGE), message);

  #undef VALIDATE_COND_WITH_MESSAGE
}

DBConnConfig_t *extract_db_conn_config_from_uri(const char *uri, const char *regex, StackErrorMessage_t *err) {
  int n = 0;
  size_t group_count = 5; // magic number 5 means the plugin regex has 5 groups. changing it could cause segfault
  char *end = NULL;
  int errornumber = 0, rc = -1;
  PCRE2_SIZE erroroffset;
  DBConnConfig_t *con_config = malloc(sizeof(DBConnConfig_t));
  char *host, *username, *port, *port_str, *password, *db, *end_ptr;

  #define MATCH_CLEANUP_FAIL(cond, err_msg)\
  {\
    if (cond) {\
      set_err((const char **)err, BUF_LEN_XS, err_msg);\
      pcre2_match_data_free(match_data);\
      pcre2_code_free(re);\
      free(con_config);\
    \
      return NULL;\
    }\
  }

  if (!con_config) return NULL;
  if (!regex || !uri) return NULL;

  pcre2_code *re = pcre2_compile(
    (PCRE2_SPTR)regex,
    PCRE2_ZERO_TERMINATED,
    0,
    &errornumber,
    &erroroffset,
    NULL
  );

  if (!re) {
    set_err((const char **)err, BUF_LEN_XS, "db connection uri regex failed to compile");
    free(con_config);

    return NULL;
  }

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  rc = pcre2_match(re, (PCRE2_SPTR)uri, strlen(uri), 0, 0, match_data, NULL);

  if (rc > 0) {
    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);

    if (rc > 1) {
      PCRE2_SIZE start_username = ovector[2], end_username = ovector[3];
      size_t len_username = end_username - start_username;

      PCRE2_SIZE start_password = ovector[4], end_password = ovector[5];
      size_t len_password = end_password - start_password;

      PCRE2_SIZE start_host = ovector[6], end_host = ovector[7];
      size_t len_host = end_host - start_host;

      PCRE2_SIZE start_port = ovector[8], end_port = ovector[9];
      size_t len_port = end_port - start_port;

      PCRE2_SIZE start_db = ovector[10], end_db = ovector[11];
      size_t len_db = end_db - start_db;

      MATCH_CLEANUP_FAIL(len_username == 0 || len_password == 0 || len_host == 0 ||
        len_port == 0 || len_db == 0, "connection uri missing a parameter");

      host = (char *)(uri + start_host), port = (char *)(uri + start_port);
      username = (char *)(uri + start_username), password = (char *)(uri + start_password),
			db = (char *)(uri + start_db);

      memcpy(con_config->username, username, len_username);
      con_config->username[len_username] = '\0';

      memcpy(con_config->name, db, len_db);
      con_config->name[len_db] = '\0';

      memcpy(con_config->password, password, len_password);
      con_config->password[len_password] = '\0';

      memcpy(con_config->host, host, len_host);
      con_config->host[len_host] = '\0';

      port_str = strdup(port);
      port_str[len_port] = '\0';

      end_ptr = NULL;
      size_t port_val = strtol(port_str, &end_ptr, 10);

      MATCH_CLEANUP_FAIL(!port_val || *end_ptr, "invalid port value");

      free(port_str);
      con_config->port = port_val;

      pcre2_match_data_free(match_data);
      pcre2_code_free(re);

      return con_config;
    }

    MATCH_CLEANUP_FAIL(true, "invalid db connection uri construction!");
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);
  free(con_config);
  set_err((const char **)err, BUF_LEN_XS, "invalid db connection uri!");

  return NULL;
  #undef MATCH_CLEANUP_FAIL
}
