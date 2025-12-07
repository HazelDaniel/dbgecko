#ifndef ___PLUGIN_MANAGER_H___
#define ___PLUGIN_MANAGER_H___

// external library headers

// standard library headers
#include <dlfcn.h>

//internal library headers
#include "globals.h"
#include "config_parser.h"

//macro defs
#define PCRE2_CODE_UNIT_WIDTH 8

#define PLUGIN_ENTRY_POINT ("___plugin_entry___")
#define PLUGIN_VOCAB_REGEX(x) "^libdriver_" #x "\\.(\\d\\.\\d)(\\.\\d)?\\.so$"
#define POSTGRES_PLUGIN_REGEX PLUGIN_VOCAB_REGEX(pg)
#define MYSQL_PLUGIN_REGEX PLUGIN_VOCAB_REGEX(mysql)
#define MONGO_PLUGIN_REGEX PLUGIN_VOCAB_REGEX(mongo)


typedef enum {
  OP_SUCCESS,
  CONNECT_FAIL,
  AUTH_FAIL,
  DATA_CORRUPT,
  FEATURE_UNSUPPORTED,
  ERR_MEM_FAILED,
  ERR_UNKNOWN,
  OP_FAIL
} DriverStatus_t;

typedef char *DriverErrMessage_t;
typedef void *PluginHandle_t;

/**
 *  PluginDriver_t - An ABI for the drivers exposed by the plugin libraries
 *  @version: the driver version
 *  @name: the driver name
 *  @init: the symbol for initializing internal states
 *  @connect: the symbol for establishing connection
 *  @backup: the symbol for backing up files
 *  @restore: the symbol for restoring files
 *  @shutdown: the symbol that releases the resources acquired by
 *    the driver on startup and unloads it
 */
typedef struct PluginDriver {
  float             version;
  char              name[BUF_LEN_XS];

  DriverStatus_t    (*init)(const AppConfig_t *app, DriverErrMessage_t *err);
  DriverStatus_t    (*connect)(const AppConfig_t *app, DriverErrMessage_t *err);
  DriverStatus_t    (*backup)(const AppConfig_t *app, DriverErrMessage_t *err);
  DriverStatus_t    (*restore)(const AppConfig_t *app, DriverErrMessage_t *err);
  DriverStatus_t    (*shutdown)(const AppConfig_t *app, DriverErrMessage_t *err);
} PluginDriver_t;

/**
 *  PluginRegistry_t - A lookup table for drivers
 *  @key: driver key, also used by uthash
 *  @driver: the instance of a driver
 */
typedef struct PluginRegistry {
  char              key[BUF_LEN_XS];
  PluginDriver_t    *driver;
  void              *handle;
  UT_hash_handle    hh;   // makes this struct hashable by uthash
} PluginRegistry_t;


PluginRegistry_t **get_plugin_registry_handle();

float extract_plugin_version_number(const char *path, const char *regex);

void print_plugin_registry(PluginRegistry_t *reg);
void set_plugin_registry(PluginRegistry_t *reg);

StackStatus_t destroy_plugin_registry(StackErrorMessage_t *err);
StackStatus_t destroy_plugin_driver(PluginDriver_t **dr, void *handle, StackErrorMessage_t *err);
PluginDriver_t *validate_plugin_ABI(PluginHandle_t handle, StackErrorMessage_t *err);
StackStatus_t register_plugin(PluginHandle_t handle, const char *key, PluginDriver_t *driver, StackErrorMessage_t *err);
StackStatus_t load_plugin(StackErrorMessage_t *err, const char *key);

DriverStatus_t to_driver_status(size_t s); // ABI level error code conversion


#endif /* ___PLUGIN_MANAGER_H___ */
