#include "include/arguments.h"
#include "include/config_parser.h"


size_t min(size_t a, size_t b) {
  return (a > b) * a + (a <= b) * b;
}

ConfigParserStatus_t config_load_file(const char *path,
  AppConfig_t *out_config, ConfigParserError_t **err) {
  FILE *fh = fopen(path, "r");
  yaml_parser_t parser;
  yaml_event_t event;
  ConfigParserStatus_t status = CONFIG_OK;
  size_t status_text_cap = BUF_LEN_S;
  char *status_text = malloc(status_text_cap);
  ConfigParserError_t *local_err = create_parser_error();
  config_section_t current_section = SECTION_NONE, parent_section = SECTION_NONE;
  parse_phase_t phase = EXPECT_KEY;
  char current_key[BUF_LEN_S] = {0}, sub_key[BUF_LEN_XS] = {0}, *set_key = NULL;
  int storage_key_set_count = 0;
  _Bool done = false;

  #define CLEANUP_ASSIGN_VALUE_FAIL() \
  {                                    \
  yaml_event_delete(&event);            \
  yaml_parser_delete(&parser);           \
  fclose(fh);                             \
  *err = local_err;                        \
  if (status_text) free(status_text);       \
  return local_err->code;                    \
  }

  if (!local_err) {
    fclose(fh);

    return status;
  }

  if (!status_text) {
    status = CONFIG_MEMORY_ERROR;
    local_err->code = status;
    snprintf(local_err->message, sizeof(local_err->message), "Config parse error: %s", "Failed to initialize status text!");
    *err = local_err;

    return status;
  }

  if (!fh) {
    status = CONFIG_FILE_NOT_FOUND;
    local_err->code = status;
    snprintf(local_err->message, sizeof(local_err->message), "Config parse error: %s", "Config file not found!");
    *err = local_err;
    if (status_text) free(status_text);

    return status;
  }

  if (!yaml_parser_initialize(&parser)) {
    fclose(fh);
    status = CONFIG_MEMORY_ERROR;
    local_err->code = status;
    snprintf(local_err->message, sizeof(local_err->message), "Config parse error: %s", "Failed to initialize parser!");
    *err = local_err;
    if (status_text) free(status_text);

    return status;
  }

  yaml_parser_set_input_file(&parser, fh);

  if (!out_config) {
    yaml_parser_delete(&parser);
    fclose(fh);
    status = CONFIG_MEMORY_ERROR;
    local_err->code = status;
    snprintf(local_err->message, sizeof(local_err->message), "Config parse error: %s", "No/invalid output config struct!");
    *err = local_err;
    if (status_text) free(status_text);

    return status;
  }

  while (!done) {
    if (!yaml_parser_parse(&parser, &event)) {
      if (err) {
        status = CONFIG_PARSE_ERROR;
        local_err->code = status;
        snprintf(status_text, status_text_cap, "YAML parse error: %s", parser.problem);
        local_err->line = parser.problem_mark.line + 1;
        local_err->column = parser.problem_mark.column + 1;
      }

      break;
    }

    switch (event.type) {
      case YAML_SCALAR_EVENT:
        if (phase == EXPECT_KEY) {
          strncpy(current_key, (char*)event.data.scalar.value, BUF_LEN_S);
          phase = EXPECT_VALUE;
        } else {
          if (current_section == SECTION_NONE) {
            if (parent_section != SECTION_STORAGE) {
              // yaml_event_delete(&event);
              continue;
            }

            if (strcmp(sub_key, "ssh") == 0 && (!storage_key_set_count || (strcmp(sub_key, set_key) == 0)) &&
              assign_yaml_parsed_value(SECTION_CHILD_SSH, current_key, (char *)event.data.scalar.value, out_config, local_err) != 0)
              { CLEANUP_ASSIGN_VALUE_FAIL(); }
            else if (strcmp(sub_key, "s3") == 0 && (!storage_key_set_count || (strcmp(sub_key, set_key) == 0)) &&
              assign_yaml_parsed_value(SECTION_CHILD_S3, current_key, (char *)event.data.scalar.value, out_config, local_err) != 0)
              { CLEANUP_ASSIGN_VALUE_FAIL(); }
            else if (strcmp(sub_key, "sftp") == 0 && (!storage_key_set_count || (strcmp(sub_key, set_key) == 0)) &&
              assign_yaml_parsed_value(SECTION_CHILD_SFTP, current_key, (char *)event.data.scalar.value, out_config, local_err) != 0)
              { CLEANUP_ASSIGN_VALUE_FAIL(); }
            else if (strcmp(sub_key, "local") == 0 && (!storage_key_set_count || (strcmp(sub_key, set_key) == 0)) &&
              assign_yaml_parsed_value(SECTION_CHILD_LOCAL, current_key, (char *)event.data.scalar.value, out_config, local_err) != 0)
              { CLEANUP_ASSIGN_VALUE_FAIL(); }
            set_key = sub_key, storage_key_set_count++;
          } else if (assign_yaml_parsed_value(current_section, current_key, (char*)event.data.scalar.value, out_config, local_err) != 0) {
            CLEANUP_ASSIGN_VALUE_FAIL();
          }
          phase = EXPECT_KEY;
        }
      break;

      case YAML_MAPPING_START_EVENT:
        if (phase == EXPECT_VALUE) {
          phase = EXPECT_KEY;
          if (strcmp(current_key, "db") == 0) current_section = SECTION_DB;
          else if (strcmp(current_key, "storage") == 0) current_section = SECTION_STORAGE;
          else if (strcmp(current_key, "runtime") == 0) current_section = SECTION_RUNTIME;
          else if (strcmp(current_key, "platform") == 0) current_section = SECTION_PLATFORM;
          else if (strcmp(current_key, "plugin") == 0) current_section = SECTION_PLUGIN;
          else {
            strcpy(sub_key, current_key);
            // printf("key: %s\n", sub_key);
            if (current_section != SECTION_NONE) parent_section = current_section;
            current_section = SECTION_NONE;
          }
          // yaml_event_delete(&event);
          continue;
        }
        break;

      case YAML_MAPPING_END_EVENT:
        if (parent_section == SECTION_STORAGE && (strcmp(sub_key, "ssh") == 0 || strcmp(sub_key, "sftp") == 0
          || strcmp(sub_key, "s3") == 0) || strcmp(sub_key, "local") == 0) {
          current_section = SECTION_STORAGE;
        } else current_section = SECTION_NONE;
        break;

      case YAML_STREAM_END_EVENT:
        done = 1;
        break;

      default:
        break;
    }

    yaml_event_delete(&event);
  }

  if (status == CONFIG_OK) {
    destroy_parser_error(&local_err);
    if (status_text) free(status_text);
  } else {
    snprintf(local_err->message, min(sizeof(local_err->message), status_text_cap), "%s", status_text);
    if (status_text) free(status_text);
    *err = local_err;
  }

  yaml_parser_delete(&parser);
  fclose(fh);

  return status;
  #undef CLEANUP_ASSIGN_VALUE_FAIL
}

void merge_configs(int argc, char **argv, StackError_t **err) {
  DBConfig_t *cfg_db = init_db_config(DEFAULT_DB_TYPE, DEFAULT_DB_URI,
    DEFAULT_DB_BACKUP_MODE, DEFAULT_DB_TIMEOUT, true);
  StorageConfig_t *cfg_storage = init_storage_config(DEFAULT_STORAGE_OUTPUT_PATH,
    DEFAULT_STORAGE_COMPRESSION, DEFAULT_STORAGE_ENC_KEY_PATH, DEFAULT_STORAGE_REMOTE);
  RuntimeConfig_t *cfg_runtime = init_runtime_config(DEFAULT_RUNTIME_LOG_LEVEL,
    DEFAULT_RUNTIME_THREAD_COUNT, DEFAULT_RUNTIME_TMP_DIR);
  PlatformConfig_t *cfg_platform = init_platform_config(DEFAULT_PLATFORM_VERSION);
  PluginConfig_t *cfg_plugin = init_plugin_config(DEFAULT_PLUGIN_DIR_PATH, DEFAULT_PLUGIN_PATH);
  AppConfig_t *cfg = init_app_config(cfg_db, cfg_storage, cfg_runtime, cfg_platform, cfg_plugin),
  **app_config = get_app_config_handle();
  ConfigParserError_t *cfg_err = NULL;
  Argument_t *parsed_args = NULL, *config_path_entry = NULL;
  ArgParserError_t *arg_err = NULL;
  FlagSchemaEntry_t *schema = NULL;
  const char *config_path = NULL;
  ArgParserStatus_t parser_status = ARG_SUCCESS;
  ConfigParserStatus_t loader_status = CONFIG_OK;
  DBConnConfig_t *db_con_config = NULL;
  StackErrorMessage_t err_msg = NULL;
  char *string_config_list[9] = {CFG_DB_PREFIX(type), CFG_DB_PREFIX(backup_mode),
    CFG_DB_PREFIX(uri), CFG_STORAGE_PREFIX(compression), CFG_STORAGE_PREFIX(remote_target),
    CFG_PATH, CFG_PLUGIN_PREFIX(dir), CFG_PLUGIN_PREFIX(path), NULL};

  set_app_config(cfg);

  for (size_t i = 0; string_config_list[i]; i++) {
    add_flag(&schema, string_config_list[i], ARG_TYPE_STRING);
  }

  add_flag(&schema, CFG_DB_PREFIX(timeout_seconds), ARG_TYPE_INT);
  add_flag(&schema, CFG_RUNTIME_PREFIX(log_level), ARG_TYPE_INT);
  add_flag(&schema, CFG_DB_PREFIX(online), ARG_TYPE_BOOL);
  add_flag(&schema, CFG_PLATFORM_PREFIX(version), ARG_TYPE_FLOAT);

  parser_status = parse_args(schema, &parsed_args, &arg_err, argc, argv);

  #define LOCAL_CLEANUP()\
  {\
    destroy_flag_schema(schema);\
    destroy_parsed_argument(parsed_args);\
    destroy_app_config();\
  }

  if (parser_status != ARG_SUCCESS) {
    if (arg_err) {
      *err = ___unsafe_to_stack_error___(arg_err);
      free(arg_err);
    }

    LOCAL_CLEANUP();

    return;
  }

  if (!parsed_args) { /* TODO: warn 'no argument provided, using platform's default */ }

  HASH_FIND_STR(parsed_args, CFG_PATH, config_path_entry);

  if (!config_path_entry) {
    *err = create_stack_error();
    strcpy((*err)->message, "no config path provided in argument");

    LOCAL_CLEANUP();

    return;
  }

  config_path = (const char *)config_path_entry->value;
  loader_status = config_load_file(config_path, cfg, &cfg_err);

  if (loader_status != CONFIG_OK) {
    if (cfg_err) {
      *err = ___unsafe_to_stack_error___(cfg_err);
      destroy_parser_error(&cfg_err);
    } else {
      *err = create_stack_error();
      strcpy((*err)->message, "An unknown error occurred while parsing the config");
    }

    LOCAL_CLEANUP();

    return;
  }

  Argument_t *current, *tmp;
  HASH_ITER(hh, parsed_args, current, tmp) {
    switch(current->type) {
      case ARG_TYPE_BOOL:
        // for now, we only support key-value pattern in arguments
        // if (strcmp(current->key, CFG_DB_PREFIX(online)) == 0) {
        //   cfg->db->online = (!current->value || strcmp(current->value, "true") == 0) ? true : false;
        // }
        break;
      case ARG_TYPE_INT:
        if (strcmp(current->key, CFG_DB_PREFIX(timeout_seconds)) == 0) {
          cfg->db->timeout_seconds = (*(size_t *)(current->value));
        } else if (strcmp(current->key, CFG_RUNTIME_PREFIX(log_level)) == 0) {
          cfg->runtime->log_level = (*(size_t *)(current->value));
        } else if (strcmp(current->key, CFG_RUNTIME_PREFIX(thread_count)) == 0) {
          cfg->runtime->thread_count = (*(size_t *)(current->value));
        }
        break;
      case ARG_TYPE_FLOAT:
        if (strcmp(current->key, CFG_PLATFORM_PREFIX(version)) == 0) {
          cfg->platform->version = (*(float *)(current->value));
        }
        break;
      case ARG_TYPE_STRING:
        if (strcmp(current->key, CFG_DB_PREFIX(type)) == 0) {
          strcpy(cfg->db->type, (char *)current->value);
        } else if (strcmp(current->key, CFG_DB_PREFIX(uri)) == 0) {
          strcpy(cfg->db->uri, (char *)current->value);
        } else if (strcmp(current->key, CFG_DB_PREFIX(backup_mode)) == 0) {
          strcpy(cfg->db->backup_mode, (char *)current->value);
        } else if (strcmp(current->key, CFG_STORAGE_PREFIX(compression)) == 0) {
          strcpy(cfg->storage->compression, (char *)current->value);
        } else if (strcmp(current->key, CFG_STORAGE_PREFIX(remote_target)) == 0) {
          strcpy(cfg->storage->remote_target, (char *)current->value);
        } else if (strcmp(current->key, CFG_STORAGE_PREFIX(output_path)) == 0) {
          strcpy(cfg->storage->output_path, (char *)current->value);
        } else if (strcmp(current->key, CFG_STORAGE_PREFIX(encryption_key_path)) == 0) {
          strcpy(cfg->storage->encryption_key_path, (char *)current->value);
        } else if (strcmp(current->key, CFG_STORAGE_PREFIX(remote_target)) == 0) {
          strcpy(cfg->storage->remote_target, (char *)current->value);
        } else if (strcmp(current->key, CFG_PLUGIN_PREFIX(dir)) == 0) {
          strcpy(cfg->plugin->dir, (char *)current->value);
        } else if (strcmp(current->key, CFG_PLUGIN_PREFIX(path)) == 0) {
          strcpy(cfg->plugin->path, (char *)current->value);
        } else if (strcmp(current->key, CFG_RUNTIME_PREFIX(tmp_dir)) == 0) {
          strcpy(cfg->runtime->temp_dir, (char *)current->value);
        }
        break;
    }
  }

  validate_app_config(*app_config, err);

  #define CLEANUP_CON_CFG_PARSE_FAIL()\
  {\
    if (!db_con_config) {\
      if (!err_msg) {\
        LOCAL_CLEANUP();\
        return;\
      }\
        \
      *err = create_stack_error();\
      strcpy((*err)->message, err_msg);\
      free(err_msg);\
      LOCAL_CLEANUP();\
      return;\
    }\
  }

  if (strcmp((*app_config)->db->type, DB_TYPE_STR_PG) == 0) {
    db_con_config = extract_db_conn_config_from_uri((*app_config)->db->uri, POSTGRES_DB_URI_REGEX, &err_msg);
    CLEANUP_CON_CFG_PARSE_FAIL();
  } else if (strcmp((*app_config)->db->type, DB_TYPE_STR_MYSQL) == 0) {
    db_con_config = extract_db_conn_config_from_uri((*app_config)->db->uri, MYSQL_DB_URI_REGEX, &err_msg);
    CLEANUP_CON_CFG_PARSE_FAIL();
  } else if (strcmp((*app_config)->db->type, DB_TYPE_STR_MONGO) == 0) {
    db_con_config = extract_db_conn_config_from_uri((*app_config)->db->uri, MONGO_DB_URI_REGEX, &err_msg);
    CLEANUP_CON_CFG_PARSE_FAIL();
  }

  snprintf((*app_config)->db->username, BUF_LEN_XS, "%s", db_con_config->username);
  snprintf((*app_config)->db->host, BUF_LEN_XS, "%s", db_con_config->host);
  snprintf((*app_config)->db->password, BUF_LEN_XS, "%s", db_con_config->password);
  (*app_config)->db->port = db_con_config->port;

  free(db_con_config);
  destroy_parsed_argument(parsed_args);
  destroy_flag_schema(schema);
  #undef LOCAL_CLEANUP
  #undef CLEANUP_CON_CFG_PARSE_FAIL
}
