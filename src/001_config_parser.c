#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include "include/config_parser.h"
#include "include/arguments.h"

size_t min(size_t a, size_t b) {
  return (a > b) * a + (a <= b) * b;
}

void print_app_config(AppConfig_t *cfg) {
  if (!cfg) return;
  puts("db:");
  printf("\t timeout_seconds: %li\n", cfg->db->timeout_seconds);
  printf("\t type: %s\n", cfg->db->type);
  printf("\t backup_mode: %s\n", cfg->db->backup_mode);
  printf("\t uri: %s\n", cfg->db->uri);
  printf("\t online: %s\n", cfg->db->online ? "true" : "false");

  puts("runtime:");
  printf("\t log_level: %li\n", cfg->runtime->log_level);
  printf("\t tmp_dir: %s\n", cfg->runtime->temp_dir);
  printf("\t thread_count: %li\n", cfg->runtime->thread_count);

  puts("storage:");
  printf("\t compression: %s\n", cfg->storage->compression);
  printf("\t key_path: %s\n", cfg->storage->encryption_key_path);
  printf("\t output_path: %s\n", cfg->storage->output_path);
  printf("\t remote_target: %s\n", cfg->storage->remote_target);

  puts("platform:");
  printf("\t version: %.2f\n", cfg->platform->version);

  puts("plugin:");
  printf("\t dir: %s\n", cfg->plugin->dir);
  printf("\t path: %s\n", cfg->plugin->path);
}

int assign_value(config_section_t section, const char *key,
  const char *value, AppConfig_t *cfg, ConfigParserError_t *err) {
  long val;
  // TODO: validate the config values

  if (section == SECTION_DB) {
    if (strcmp(key, "type") == 0) strncpy(cfg->db->type, value, BUF_LEN_XS);
    else if (strcmp(key, "uri") == 0) strncpy(cfg->db->uri, value, BUF_LEN_S);
    else if (strcmp(key, "backup_mode") == 0) strncpy(cfg->db->backup_mode, value, BUF_LEN_XS);
    else if (strcmp(key, "online") == 0 ) {
      if (strcmp(value, "true") == 0) cfg->db->online = true;
      else if (strcmp(value, "false") == 0) cfg->db->online = false;
    } else if (strcmp(key, "timeout_seconds") == 0) {
      val = strtol(value, NULL, 10);

      if (val <= 0) {
        err->code = CONFIG_VALIDATION_ERROR;
        snprintf(err->message, sizeof(err->message), "db->timeout_seconds must be > 0");

        return -1;
      }

      cfg->db->timeout_seconds = (int)val;
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown db key: %s", key);

      return -1;
    }
  } else if (section == SECTION_STORAGE) {
    if (strcmp(key, "output_path") == 0) strncpy(cfg->storage->output_path, value, BUF_LEN_S);
    else if (strcmp(key, "compression") == 0) strncpy(cfg->storage->compression, value, BUF_LEN_XS);
    else if (strcmp(key, "remote_target") == 0) strncpy(cfg->storage->remote_target, value, BUF_LEN_XS);
    else if (strcmp(key, "encryption_key_path") == 0) strncpy(cfg->storage->encryption_key_path, value, BUF_LEN_S);
    else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown storage key: %s", key);

      return -1;
    }
  } else if (section == SECTION_PLATFORM) {
    if (strcmp(key, "version") == 0) {
      char *end = NULL;
      val = strtof(value, &end);
      if (*end != '\0') {
        err->code = CONFIG_VALIDATION_ERROR;
        snprintf(err->message, sizeof(err->message), "Invalid platform version: %s", value);

        return -1;
      }
      cfg->platform->version = val;
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown platform key: %s", key);

      return -1;
    }
  } else if (section == SECTION_PLUGIN) {
    if (strcmp(key, "dir") == 0) {
       strncpy(cfg->plugin->dir, value, BUF_LEN_S);
    } else if (strcmp(key, "path") == 0) {
       strncpy(cfg->plugin->path, value, BUF_LEN_M);
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown plugin key: %s", key);

      return -1;
    }
  } else if (section == SECTION_RUNTIME) {
    if (strcmp(key, "log_level") == 0) {
      val = strtol(value, NULL, 10);
      cfg->runtime->log_level = (int)val;
    } else if (strcmp(key, "thread_count") == 0) {
      val = strtol(value, NULL, 10);
      cfg->runtime->thread_count = (int)val;
    } else if (strcmp(key, "tmp_dir") == 0) {
      strncpy(cfg->runtime->temp_dir, value, BUF_LEN_S);
    } else {
      err->code = CONFIG_VALIDATION_ERROR;
      snprintf(err->message, sizeof(err->message), "Unknown runtime key: %s", key);

      return -1;
    }
  }

  return 0;
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
  config_section_t current_section = SECTION_NONE;
  parse_phase_t phase = EXPECT_KEY;
  char current_key[BUF_LEN_S] = {0};
  _Bool done = false;

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
          if (assign_value(current_section, current_key, (char*)event.data.scalar.value, out_config, local_err) != 0) {
            yaml_event_delete(&event);
            yaml_parser_delete(&parser);
            fclose(fh);
            *err = local_err;
            if (status_text) free(status_text);

            return local_err->code;
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
          else current_section = SECTION_NONE;
          // yaml_event_delete(&event);
          continue;
        }
        break;

      case YAML_MAPPING_END_EVENT:
        current_section = SECTION_NONE;
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

  destroy_parsed_argument(parsed_args);
  destroy_flag_schema(schema);
  #undef LOCAL_CLEANUP
}
