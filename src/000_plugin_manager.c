#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include "include/plugin_manager.h"
#include <pcre2.h>


static PluginRegistry_t *plugin_registry = NULL;

PluginRegistry_t **get_plugin_registry_handle() {
  return &plugin_registry;
}

PluginRegistry_t *get_plugin_registry_entry(const char *const key) {
  PluginRegistry_t *plugin_entry = NULL, *plugin_head = *get_plugin_registry_handle();
  AppConfig_t *app = *get_app_config_handle();

  if (!plugin_head) return NULL;
  HASH_FIND_STR(plugin_head, key ? key : app->db->type , plugin_entry);

  return plugin_entry;
}

void set_plugin_registry(PluginRegistry_t *reg) {
  PluginRegistry_t **reg_ptr = get_plugin_registry_handle();
  char *error = NULL;

  if (!reg_ptr) return;
  if (*reg_ptr) destroy_plugin_registry(&error);
  if (error) free(error);
  *reg_ptr = reg;
}

float extract_plugin_version_number(const char *path, const char *regex) {
  float version = -1.0;
  int n = 0;
  size_t group_count = 2; // magic number 2 means the plugin regex has 2 groups. changing it could cause segfault
  char *end = NULL;
  int errornumber = 0, rc = -1;
  PCRE2_SIZE erroroffset;

  if (!regex || !path) return version;

  pcre2_code *re = pcre2_compile(
    (PCRE2_SPTR)regex,
    PCRE2_ZERO_TERMINATED,
    0,
    &errornumber,
    &erroroffset,
    NULL
  );

  if (!re) {
    return -1.0f;
  }

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  rc = pcre2_match(re, (PCRE2_SPTR)path, strlen(path), 0, 0, match_data, NULL);

  if (rc > 0) {
    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);

    if (rc > 2) {
      PCRE2_SIZE start = ovector[4]; // second group start (the float parsable part \d\.\d)
      PCRE2_SIZE end = ovector[5]; // second group end
      size_t len = end - start;

      char buf[BUF_LEN_XS];
      if (len >= sizeof(buf)) len = sizeof(buf) - 1;
      memcpy(buf, path + start, len);
      buf[len] = '\0';
      char *endptr = NULL;
      version = strtof(buf, &endptr);
      if (*endptr != '\0') version = -1.0f;
    }
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  return version;
}

void extract_plugin_version_string(const char *path, const char *regex, char *out, size_t out_len) {
  int n = 0;
  size_t group_count = 2; // magic number 2 means the plugin regex has 2 groups
  int errornumber = 0, rc = -1;
  PCRE2_SIZE erroroffset;

  if (!regex || !path || !out || out_len == 0) return;
  out[0] = '\0';

  pcre2_code *re = pcre2_compile(
    (PCRE2_SPTR)regex,
    PCRE2_ZERO_TERMINATED,
    0,
    &errornumber,
    &erroroffset,
    NULL
  );

  if (!re) return;

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);
  rc = pcre2_match(re, (PCRE2_SPTR)path, strlen(path), 0, 0, match_data, NULL);

  if (rc > 1) {
    PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
    PCRE2_SIZE start = ovector[2]; // first group start
    PCRE2_SIZE end = ovector[3]; // first group end
    size_t len = end - start;

    if (len >= out_len) len = out_len - 1;
    memcpy(out, path + start, len);
    out[len] = '\0';
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);
}

PluginDriver_t *validate_plugin_ABI(PluginHandle_t handle, StackErrorMessage_t *err) {
  PluginDriver_t *driver = NULL, *(*driver_wrapper)(void);
  char *dl_error = NULL;
  AppConfig_t *cfg = *get_app_config_handle();

  #define LOCAL_CLEANUP_HANDLE() \
  {                               \
    dlerror();                     \
    dlclose(handle);                \
    dl_error = dlerror();            \
                                      \
    if (dl_error && !*err) {           \
      *err = malloc(BUF_LEN_S * sizeof(char));                                        \
      snprintf(*err, BUF_LEN_S, "failed to close a plugin handle!. %s\n", dl_error);   \
    }                                                                                    \
    \
  }


  dlerror();
  driver_wrapper = dlsym(handle, PLUGIN_ENTRY_POINT);
  dl_error = dlerror();

  if (dl_error || !driver_wrapper) {
    *err = malloc(BUF_LEN_S * sizeof(char));
    if (dl_error) snprintf(*err, BUF_LEN_S, "plugin invalid!. %s\n", dl_error);
    else snprintf(*err, BUF_LEN_XS, "plugin invalid! entry point not found");
    LOCAL_CLEANUP_HANDLE();

    return NULL;
  }

  driver = driver_wrapper();

  if (!driver) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    snprintf(*err, BUF_LEN_XS, "plugin invalid! driver not found");
    LOCAL_CLEANUP_HANDLE();

    return NULL;
  }

  if (!driver->init || !driver->backup || !driver->connect || !driver->restore || !driver->shutdown) {
    *err = malloc(BUF_LEN_S * sizeof(char));

    if (!driver->init) {
      snprintf(*err, BUF_LEN_S, "plugin invalid! driver missing 'init' field");
      if (driver->shutdown) driver->shutdown(cfg, err);
    } else if (!driver->backup) {
       snprintf(*err, BUF_LEN_S, "plugin invalid! driver missing 'backup' field");
       if (driver->shutdown) driver->shutdown(cfg, err);
    } else if (!driver->connect) {
      snprintf(*err, BUF_LEN_S, "plugin invalid! driver missing 'connect' field");
      if (driver->shutdown) driver->shutdown(cfg, err);
    } else if (!driver->restore) {
      snprintf(*err, BUF_LEN_S, "plugin invalid! driver missing 'restore' field");
      if (driver->shutdown) driver->shutdown(cfg, err);
    } else if (!driver->shutdown) snprintf(*err, BUF_LEN_S, "plugin invalid! driver missing 'shutdown' field");

    LOCAL_CLEANUP_HANDLE();

    return NULL;
  }

  if (driver->version < CURRENT_PLATFORM_VERSION - VERSION_SUPPORT_RANGE
    || driver->version > CURRENT_PLATFORM_VERSION + VERSION_SUPPORT_RANGE) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    snprintf(*err, BUF_LEN_XS, "plugin invalid! unsupported version (%.1f).", driver->version);
    // driver->shutdown(err); // TODO: take care of this later
    LOCAL_CLEANUP_HANDLE();

    return NULL;
  }

  return driver;
  #undef LOCAL_CLEANUP_HANDLE
}

StackStatus_t destroy_plugin_driver(PluginDriver_t **dr, void *handle, StackErrorMessage_t *err) {
  DriverStatus_t status = OP_SUCCESS;
  PluginDriver_t *driver = NULL;
  char *dlerr = NULL;
  AppConfig_t *app_cfg = *get_app_config_handle();
  // TODO: implement warning on error paths instead.shouldn't affect normal program flow
  #define LOCAL_CLEANUP()           \
  {                                   \
                                        \
    dlerror(); dlclose(handle);           \
    dlerr = dlerror();                      \
                                              \
    if (dlerr) {                                \
      *err = malloc(BUF_LEN_S * sizeof(char));    \
      sprintf(*err, "driver '%s's handle failed to close. reason: %s \n", driver->name, dlerr); \
    }             \
    *dr = NULL;     \
  }

  if (!dr || !*dr) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    sprintf(*err, "couldn't close plugin. driver '%s' doesn't exist \n", driver->name);

    return EXEC_FAILURE;
  }

  driver = *dr;
  status = to_driver_status(driver->shutdown(app_cfg, err));

  if (status != OP_SUCCESS) {
    if (*err) return EXEC_FAILURE;
    *err = malloc(BUF_LEN_S * sizeof(char));
    sprintf(*err, "couldn't close plugin. driver '%s' couldn't shutdown \n", driver->name);
    LOCAL_CLEANUP()

    return EXEC_FAILURE;
  }

  LOCAL_CLEANUP()

  return EXEC_SUCCESS;
  #undef LOCAL_CLEANUP
}

StackStatus_t destroy_plugin_registry(StackErrorMessage_t *err) {
  PluginRegistry_t **registry_ref = get_plugin_registry_handle();
  DriverErrMessage_t driver_err = NULL;
  DriverStatus_t status = OP_SUCCESS;

  if (!*registry_ref) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    strncpy(*err, "couldn't destroy plugin registry. none allocated\n", BUF_LEN_XS);
    *err[BUF_LEN_XS - 1] = '\0';

    return EXEC_FAILURE;
  }

  PluginRegistry_t *current, *tmp;

  HASH_ITER(hh, *registry_ref, current, tmp) {
    HASH_DEL(*registry_ref, current);
    if (current->driver) {
      status = to_driver_status(destroy_plugin_driver(&current->driver, current->handle, err));
      if (status != OP_SUCCESS) {
        return EXEC_FAILURE;
      }
    }
    free(current);
  }

  *registry_ref = NULL;

  return EXEC_SUCCESS;
}
