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

DBConfig_t *init_db_config(const char *type, const char *uri, const char *backup_mode, size_t timeout_seconds) {
  DBConfig_t *cfg = malloc(sizeof(DBConfig_t));

  if (!cfg) return NULL;
  strncpy(cfg->type, type, sizeof(cfg->type) - 1);
  cfg->type[sizeof(cfg->type) - 1] = '\0';
  strncpy(cfg->uri, uri, sizeof(cfg->uri) - 1);
  strncpy(cfg->backup_mode, backup_mode, sizeof(cfg->backup_mode) - 1);
  cfg->uri[sizeof(cfg->uri) - 1] = '\0';
  cfg->timeout_seconds = timeout_seconds;
  cfg->max_retries = DEFAULT_DB_MAX_RETRIES;
  cfg->online = false;

  return cfg;
}

StorageConfig_t *init_storage_config(const char *output_name, const char *compression, const char *encryption_key_path, const char *remote_target) {
  StorageConfig_t *cfg = malloc(sizeof(StorageConfig_t));

  if (!cfg) return NULL;
  cfg->backend = malloc(sizeof(StorageBackendConfig_t));
  if (!cfg->backend) return NULL;

  strncpy(cfg->output_name, output_name, sizeof(cfg->output_name) - 1);
  cfg->output_name[sizeof(cfg->output_name) - 1] = '\0';
  strncpy(cfg->compression, compression, sizeof(cfg->compression) - 1);
  cfg->compression[sizeof(cfg->compression) - 1] = '\0';
  strncpy(cfg->encryption_key_path, encryption_key_path, sizeof(cfg->encryption_key_path) - 1);
  cfg->encryption_key_path[sizeof(cfg->encryption_key_path) - 1] = '\0';
  strncpy(cfg->remote_target, remote_target, sizeof(cfg->remote_target) - 1);
  cfg->remote_target[sizeof(cfg->remote_target) - 1] = '\0';
  cfg->backend->kind = PTC_UNKNOWN;

  return cfg;
}

RuntimeConfig_t *init_runtime_config(size_t log_level, size_t thread_count, const char *temp_dir, const char *mode, const char *op) {
  RuntimeConfig_t *cfg = malloc(sizeof(RuntimeConfig_t));

  if (!cfg) return NULL;
  cfg->log_level = log_level;
  cfg->thread_count = thread_count;
  strncpy(cfg->temp_dir, temp_dir, sizeof(cfg->temp_dir) - 1);
  cfg->temp_dir[sizeof(cfg->temp_dir) - 1] = '\0';
  strncpy(cfg->mode, mode ? mode : "tui", sizeof(cfg->mode) - 1);
  cfg->mode[sizeof(cfg->mode) - 1] = '\0';
  strncpy(cfg->op, op ? op : "", sizeof(cfg->op) - 1);
  cfg->op[sizeof(cfg->op) - 1] = '\0';

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
  #define VALIDATE_COND_WITH_MESSAGE(str, cond, ...) \
  {\
    message = str;\
    if (cond) { \
      *err = create_stack_error(); \
      snprintf((*err)->message, BUF_LEN_SS, __VA_ARGS__, message); \
      if (cfg) destroy_app_config(); \
      return; \
    } \
    \
  }

  char *message = NULL;

  VALIDATE_COND_WITH_MESSAGE("config doesn't exist!", (!cfg), "%s");
  VALIDATE_COND_WITH_MESSAGE("invalid platform version!", (!cfg->platform->version), "%s");
  VALIDATE_COND_WITH_MESSAGE("invalid timeout seconds value for db!", (!cfg->db->timeout_seconds), "%s");
  VALIDATE_COND_WITH_MESSAGE("invalid max retries value for db!", (!cfg->db->max_retries), "%s");
  VALIDATE_COND_WITH_MESSAGE("backup mode invalid!", (!cfg->db->backup_mode[0] || (strcmp(cfg->db->backup_mode, "incremental") != 0 &&\
    strcmp(cfg->db->backup_mode, "full") != 0 && strcmp(cfg->db->backup_mode, "schema-only") != 0)), "%s");
  VALIDATE_COND_WITH_MESSAGE("invalid db type!", (strcmp(cfg->db->type, DB_TYPE_STR_PG) != 0 &&\
    strcmp(cfg->db->type, DB_TYPE_STR_MYSQL) != 0 && strcmp(cfg->db->type, DB_TYPE_STR_MONGO) != 0), "%s");
  VALIDATE_COND_WITH_MESSAGE("backup mode invalid!", (!cfg->db->uri[0]), "%s");
  VALIDATE_COND_WITH_MESSAGE("output name not provided!", (!cfg->storage->output_name[0]), "%s");
  // VALIDATE_COND_WITH_MESSAGE("encryption key path not provided!", (cfg->db->online && !cfg->storage->encryption_key_path[0]), "%s");
  // VALIDATE_COND_WITH_MESSAGE("remote location not provided!", (cfg->db->online && !cfg->storage->remote_target[0]), "%s");
  VALIDATE_COND_WITH_MESSAGE("invalid log level!", (!cfg->runtime->log_level), "%s");
  VALIDATE_COND_WITH_MESSAGE("invalid thread count!", (!cfg->runtime->thread_count), "%s");
  VALIDATE_COND_WITH_MESSAGE("temporary runtime directory not provided!", (!cfg->runtime->temp_dir[0]), "%s");
  VALIDATE_COND_WITH_MESSAGE("incompatible platform version", (cfg->platform->version < CURRENT_PLATFORM_VERSION - VERSION_SUPPORT_RANGE ||
    cfg->platform->version > CURRENT_PLATFORM_VERSION + VERSION_SUPPORT_RANGE), "%s");
  VALIDATE_COND_WITH_MESSAGE("invalid mode!", (strcmp(cfg->runtime->mode, "tui") != 0 && strcmp(cfg->runtime->mode, "cli") != 0), "%s");
  VALIDATE_COND_WITH_MESSAGE("operation not provided in CLI mode!", (strcmp(cfg->runtime->mode, "cli") == 0 && !cfg->runtime->op[0]), "%s");

  #define STORAGE_CFG_KEY_MISSING_CHECK(backend_, key_) \
  {\
    if (!cfg->storage->backend->backend.backend_.key_[0]) {\
      *err = create_stack_error();\
      snprintf((*err)->message, BUF_LEN_SS, "storage '" #backend_ "' backend config missing key '" #key_ "'");\
      if (cfg) destroy_app_config();\
      return;\
    }\
  }

  switch (cfg->storage->backend->kind) {
    case PTC_S3:
      STORAGE_CFG_KEY_MISSING_CHECK(s3, endpoint); STORAGE_CFG_KEY_MISSING_CHECK(s3, region);
      STORAGE_CFG_KEY_MISSING_CHECK(s3, access_key); STORAGE_CFG_KEY_MISSING_CHECK(s3, secret_key);
      STORAGE_CFG_KEY_MISSING_CHECK(s3, path_style); STORAGE_CFG_KEY_MISSING_CHECK(s3, session_token);
      break;
    case PTC_SFTP:
      STORAGE_CFG_KEY_MISSING_CHECK(sftp, private_key); STORAGE_CFG_KEY_MISSING_CHECK(sftp, host);
      STORAGE_CFG_KEY_MISSING_CHECK(sftp, username);
      break;
    case PTC_LOCAL:
      break;
    default:
      *err = create_stack_error();
      snprintf((*err)->message, BUF_LEN_SS, "no storage backend provided!");
      if (cfg) destroy_app_config();
      return;
  }

  #undef VALIDATE_COND_WITH_MESSAGE
  #undef STORAGE_CFG_KEY_MISSING_CHECK
}

DBConnConfig_t *extract_db_conn_config_from_uri(const char *uri, const char *regex, StackErrorMessage_t *err) {
  _Bool mongo_mode = strcmp(regex, MONGO_DB_URI_REGEX) == 0;
  int n = 0;
  size_t group_count = mongo_mode ? 6 : 5; // magic number (6|5) means the plugin regex has (6|5) groups. changing it could cause segfault
  int errornumber = 0, rc = -1;
  PCRE2_SIZE erroroffset;
  char *host, *username, *port, *port_str, *password, *db, *host_port_seq, *opt_list, *end_ptr;
  DBConnConfig_t *con_config = malloc(sizeof(DBConnConfig_t));

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
  con_config->online = true;

  if (rc > 0) {
    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);

    if (mongo_mode && rc > 1) {
      con_config->host[0] = '\0', con_config->username[0] = '\0',
      con_config->password[0] = '\0', con_config->name[0] = '\0';

      PCRE2_SIZE start_db = ovector[10], end_db = ovector[11];
      size_t len_db = end_db - start_db;

      MATCH_CLEANUP_FAIL(len_db == 0, "connection uri missing 'db' parameter");

      PCRE2_SIZE start_host_and_port = ovector[6], end_host_and_port = ovector[7];
      size_t len_host_and_port = end_host_and_port - start_host_and_port;

      host_port_seq = (char *)(uri + start_host_and_port);
      memcpy(con_config->host, host_port_seq, len_host_and_port);
      con_config->host[len_host_and_port] = '\0';

      if (strcmp(con_config->host, "127.0.0.1") == 0 || strcmp(con_config->host, "localhost") == 0) {
        con_config->online = false;
      }

      pcre2_match_data_free(match_data);
      pcre2_code_free(re);

			return con_config;
    } else if (rc > 1) {
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

      if (strcmp(con_config->host, "127.0.0.1") == 0 || strcmp(con_config->host, "localhost") == 0)
        con_config->online = false;

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
