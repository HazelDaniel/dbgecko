#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include "include/plugin_manager.h"


static PluginRegistry_t *plugin_registry = NULL;

PluginRegistry_t **get_plugin_registry_handle() {
  return &plugin_registry;
}

StackStatus_t destroy_plugin_driver(PluginDriver_t **dr, void *handle, StackErrorMessage_t *err) {
  DriverStatus_t status = OP_SUCCESS;
  PluginDriver_t *driver = NULL;
  char *dlerr = NULL;
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
    free(driver);   \
    *dr = NULL;       \
  }

  if (!dr || !*dr) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    sprintf(*err, "couldn't close plugin. driver '%s' doesn't exist \n", driver->name);

    return EXEC_FAILURE;
  }

  driver = *dr;
  status = to_driver_status(driver->shutdown(err));

  if (status != OP_SUCCESS) {
    if (*err) return EXEC_FAILURE;
    *err = malloc(BUF_LEN_S * sizeof(char));
    sprintf(*err, "couldn't close plugin. driver '%s' couldn't shutdown \n", driver->name);
    LOCAL_CLEANUP()

    return EXEC_FAILURE;
  }

  LOCAL_CLEANUP()
  #undef LOCAL_CLEANUP

  return EXEC_SUCCESS;
}

StackStatus_t destroy_plugin_registry(StackErrorMessage_t *err) {
  PluginRegistry_t *registry = NULL, **registry_ref = get_plugin_registry_handle();
  DriverErrMessage_t driver_err = NULL;
  DriverStatus_t status = OP_SUCCESS;

  registry = *registry_ref;

  if (!registry) {
    *err = malloc(BUF_LEN_XS * sizeof(char));
    strncpy(*err, "couldn't destroy plugin registry. none allocated\n", BUF_LEN_XS);
    *err[BUF_LEN_XS - 1] = '\0';

    return EXEC_FAILURE;
  }

  PluginRegistry_t *current, *tmp;

  HASH_ITER(hh, registry, current, tmp) {
    HASH_DEL(registry, current);
    free(current->key);
    if (current->driver) {
      status = to_driver_status(destroy_plugin_driver(&current->driver, current->handle, err));
      if (status != OP_SUCCESS) {
        return EXEC_FAILURE;
      }
    }
    free(current);
  }

  free(registry);
  *registry_ref = NULL;

  return EXEC_SUCCESS;
}
