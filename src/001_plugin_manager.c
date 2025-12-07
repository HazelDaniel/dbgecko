#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "include/plugin_manager.h"
#include <pcre2.h>


StackStatus_t register_plugin(PluginHandle_t handle, const char *key, PluginDriver_t *driver, StackErrorMessage_t *err) {
  StackStatus_t status = EXEC_SUCCESS;
  PluginRegistry_t **registry_ptr = get_plugin_registry_handle(), *registry = NULL;
  DriverStatus_t driver_status = OP_SUCCESS;
  AppConfig_t *cfg = *get_app_config_handle();

  registry = malloc(sizeof(PluginRegistry_t));
  registry->driver = driver;
  registry->handle = handle;
  strncpy(registry->key, key, BUF_LEN_XS);
  HASH_ADD_STR(*registry_ptr, key, registry);

  driver_status = registry->driver->init(cfg, err); // initialize plugin after registering

  if (driver_status != OP_SUCCESS) {
    if (!*err) {
      *err = malloc(BUF_LEN_XS * sizeof(char));
      snprintf(*err, BUF_LEN_XS, "driver init call failed");

      return EXEC_FAILURE;
    }
  }

  return status;
}

StackStatus_t get_key_res_regex(StackErrorMessage_t *err, const char res_regex[BUF_LEN_XS], const char *key) {
  StackStatus_t status = EXEC_SUCCESS;
  AppConfig_t *cfg = *get_app_config_handle();

  if (!key) {
    if (strcmp(cfg->db->type, DB_TYPE_STR_PG) == 0) {
      snprintf((char *)res_regex, BUF_LEN_XS, POSTGRES_PLUGIN_REGEX);
    } else if (strcmp(cfg->db->type, DB_TYPE_STR_MYSQL) == 0) {
      snprintf((char *)res_regex, BUF_LEN_XS, MYSQL_PLUGIN_REGEX);
    } else if (strcmp(cfg->db->type, DB_TYPE_STR_MONGO) == 0) {
      snprintf((char *)res_regex, BUF_LEN_XS, MONGO_PLUGIN_REGEX);
    } else {
      *err = malloc(BUF_LEN_XS * sizeof(char));
      if (key) snprintf(*err, BUF_LEN_XS, "unknown plugin type: '%s'!", key);
      else snprintf(*err, BUF_LEN_XS, "unknown plugin type!");
      status = EXEC_FAILURE;

      return status;
    }
  } else {
    if (strcmp(key, DB_TYPE_STR_PG) == 0) {
      snprintf((char *)res_regex, BUF_LEN_XS, POSTGRES_PLUGIN_REGEX);
    } else if (strcmp(key, DB_TYPE_STR_MYSQL) == 0) {
      snprintf((char *)res_regex, BUF_LEN_XS, MYSQL_PLUGIN_REGEX);
    } else if (strcmp(key, DB_TYPE_STR_MONGO) == 0) {
      snprintf((char *)res_regex, BUF_LEN_XS, MONGO_PLUGIN_REGEX);
    } else {
      *err = malloc(BUF_LEN_XS * sizeof(char));
      if (key) snprintf(*err, BUF_LEN_XS, "unknown plugin type: '%s'!", key);
      else snprintf(*err, BUF_LEN_XS, "unknown plugin type!");
      status = EXEC_FAILURE;

      return status;
    }
  }

  return status;
}

StackStatus_t load_candidate_path_plugin(const char *res_path, const char *key, StackErrorMessage_t *err) {
  PluginHandle_t plugin_handle = NULL;
  char *dl_error = NULL;
  PluginDriver_t *driver = NULL;
  StackStatus_t status = EXEC_SUCCESS;

  dlerror();
  plugin_handle = dlopen(res_path, RTLD_NOW | RTLD_LOCAL);
  dl_error = dlerror();

  if (dl_error) {
    *err = malloc(BUF_LEN_S * sizeof(char));
    snprintf(*err, BUF_LEN_S, "couldn't load plugin!. %s\n", dl_error);
    status = EXEC_FAILURE;

    return status;
  }

  driver = validate_plugin_ABI(plugin_handle, err);

  if (!driver) return EXEC_FAILURE;

  status = register_plugin(plugin_handle, key, driver, err);

  if (status != EXEC_SUCCESS) {
    if (!*err) {
      *err = malloc(BUF_LEN_XS * sizeof(char));
      snprintf(*err, BUF_LEN_XS, "failed to register plugin '%s' !", key);
    }

    status = EXEC_FAILURE;

    return status;
  }

  return status;
}

StackStatus_t load_plugin_scan(StackErrorMessage_t *err, AppConfig_t *cfg, const char *key) {
  StackStatus_t status = EXEC_SUCCESS;
  struct dirent *entry = NULL;
  char fullpath[BUF_LEN_L], res_regex[BUF_LEN_XS] = {0}, candidate_path[BUF_LEN_L];
  struct stat st;
  int errornumber, rc = -1;
  PCRE2_SIZE erroroffset;
  pcre2_match_data *match_data = NULL;
  float candidate_version = CURRENT_PLATFORM_VERSION, curr_version = -1.0f;
  DIR *dir = NULL;

  if (cfg->plugin->dir[0] == '\0') {
    *err = malloc(BUF_LEN_S * sizeof(char));
    snprintf(*err, BUF_LEN_S, "couldn't load plugin. No directory provided");
    status = EXEC_FAILURE;
    closedir(dir);

    return status;
  }

  dir = opendir(cfg->plugin->dir);

  if (!dir) {
    *err = malloc(BUF_LEN_S * sizeof(char));
    snprintf(*err, BUF_LEN_S, "couldn't load plugin from '%s'!. No such file or directory", cfg->plugin->dir);
    status = EXEC_FAILURE;

    return status;
  }

  status = get_key_res_regex(err, res_regex, key);
  if (status != EXEC_SUCCESS) {
    closedir(dir);

    return status;
  }

  pcre2_code *re = pcre2_compile(
    (PCRE2_SPTR)res_regex,
    PCRE2_ZERO_TERMINATED,
    0,
    &errornumber,
    &erroroffset,
    NULL
  );

  if (!re) {
    PCRE2_UCHAR buffer[BUF_LEN_S];
    pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
    *err = malloc(BUF_LEN_S * sizeof(char));
    snprintf(*err, BUF_LEN_S, "plugin regex compilation failed at %zu: '%s'.", erroroffset, (char *)buffer);
    status = EXEC_FAILURE;
    closedir(dir);

    return status;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    snprintf(fullpath, sizeof(fullpath), "%s", cfg->plugin->dir);

    if (fullpath[strlen(fullpath) - 1] == '/') {/* TODO: use platform-agnostic path delimiters */
      snprintf(fullpath + strlen(fullpath), sizeof(fullpath) - strlen(fullpath), "%s", entry->d_name);
    } else {
      snprintf(fullpath + (strlen(fullpath)), sizeof(fullpath) - strlen(fullpath), "/%s", entry->d_name);
    }

    if (stat(fullpath, &st) != 0 || !S_ISREG(st.st_mode)) continue;
    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    rc = pcre2_match(
      re,                              // the compiled pattern
      (PCRE2_SPTR)entry->d_name,       // subject string
      strlen(entry->d_name),           // length of subject
      0,                               // start at offset 0
      0,                               // default options
      match_data,                      // match data block
      NULL
    );

    if (rc < 0) {
      pcre2_match_data_free(match_data);
      continue;
    }

    curr_version = extract_plugin_version_number(entry->d_name, res_regex);

    if (curr_version < 0) {
      pcre2_match_data_free(match_data);
      continue;
    }

    if (curr_version >= CURRENT_PLATFORM_VERSION - VERSION_SUPPORT_RANGE
      && curr_version <= CURRENT_PLATFORM_VERSION + VERSION_SUPPORT_RANGE
      && curr_version > candidate_version) {
      candidate_version = curr_version;
      snprintf(candidate_path, sizeof(fullpath), "%s", fullpath);
    }

    pcre2_match_data_free(match_data);
  }

  status = load_candidate_path_plugin(candidate_path, key, err);

  if (status != EXEC_SUCCESS)  {
    pcre2_code_free(re); closedir(dir);

    return status;
  }

  pcre2_code_free(re); closedir(dir);

  return status;
}

StackStatus_t load_plugin_seek(StackErrorMessage_t *err, AppConfig_t *cfg, const char *key, const char *res_path) {
  StackStatus_t status = EXEC_SUCCESS;
  float version = -1.0f;
  char res_regex[BUF_LEN_XS] = {0};

  status = get_key_res_regex(err, res_regex, key);
  if (status != EXEC_SUCCESS) return status;
  version = extract_plugin_version_number(cfg->plugin->path, res_regex);

  if (version < 0) {
    *err = malloc(BUF_LEN_S * sizeof(char));
    if (key) snprintf(*err, BUF_LEN_S, "couldn't load plugin '%s'!. .so type mismatch or invalid .so naming", key);
    else snprintf(*err, BUF_LEN_S, "couldn't load plugin!. .so type mismatch or invalid .so naming");
    status = EXEC_FAILURE;

    return status;
  }

  status = load_candidate_path_plugin(res_path, key, err);

  if (status != EXEC_SUCCESS) return status;

  return status;
}

StackStatus_t load_plugin(StackErrorMessage_t *err, const char *key_) {
  StackStatus_t status = EXEC_SUCCESS;
  char *key = NULL, *res_path = NULL, *dl_error = NULL;
  AppConfig_t **cfg_ptr = get_app_config_handle(), *cfg = NULL;
  _Bool random_search = false;
  float version = -1.0f;
  const char *res_regex = NULL;
  PluginHandle_t plugin_handle = NULL;
  PluginDriver_t *driver = NULL;

  cfg = *cfg_ptr;

  if (!cfg) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    if (key_) snprintf(*err, BUF_LEN_XS, "couldn't load plugin '%s'! config not set", key_);
    else snprintf(*err, BUF_LEN_XS, "couldn't load plugin! config not set");
    status = EXEC_FAILURE;

    return status;
  }

  if (!cfg->plugin->path[0]) {
    random_search = true;
  } else {
    res_path = malloc(BUF_LEN_M * sizeof(char));
    snprintf(res_path, sizeof(cfg->plugin->dir), "%s", cfg->plugin->dir);
    if (res_path[strlen(res_path) - 1] != '/' && cfg->plugin->path[0] != '/') {/* TODO: use platform-agnostic path delimiters */
      res_path[strlen(res_path)] = '/';
    }
    strncat(res_path, cfg->plugin->path, BUF_LEN_M - 1);
  }

  key = key_ ? (char *)(key_) : cfg->db->type;

  if (!random_search) {
    status = load_plugin_seek(err, cfg, key, res_path);

    if (status != EXEC_SUCCESS && !*err) {
      *err = malloc(BUF_LEN_XS * sizeof(char));
      snprintf(*err, BUF_LEN_XS, "failed to locate plugin '%s' !", key);
      if (res_path) free(res_path);
      status = EXEC_FAILURE;

      return status;
    }

    if (res_path) free(res_path);

    return status;
  }

  status = load_plugin_scan(err, cfg, key);

  if (status != EXEC_SUCCESS && !*err) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    snprintf(*err, BUF_LEN_XS, "failed to locate plugin '%s' !", key);
    if (res_path) free(res_path);
    status = EXEC_FAILURE;

    return status;
  }

  if (res_path) free(res_path);

  return status;
}
