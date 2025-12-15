#include <stdio.h>
#include "include/config_parser.h"  // your header for AppConfig_t, config_load_file, etc.

/**
  * -------------------AI-GENERATED CODE----------------------
*/

int main(int argc, char *argv[]) {
    if (argc < 2) {
      fprintf(stderr, "Usage: %s <config.yml>\n", argv[0]);
      return 1;
    }

    const char *config_path = argv[1];
    AppConfig_t **cfg_ptr = get_app_config_handle(), *cfg = NULL;
    DBConfig_t *cfg_db = init_db_config("passed:postgres", "passed:https://db", "passed:full", 5000);
    StorageConfig_t *cfg_storage = init_storage_config("passed:./tests", "passed:shannon", "passed:./pat", "passed:http://remote");
    RuntimeConfig_t *cfg_runtime = init_runtime_config(3, 2, "passed:locals");
    PlatformConfig_t *cfg_platform = init_platform_config(2.0);
    PluginConfig_t *cfg_plugin = init_plugin_config("passed:/plugins", "passed:/plugins/lib_pg.1.0.so");

    cfg = init_app_config(cfg_db, cfg_storage, cfg_runtime, cfg_platform, cfg_plugin);
    set_app_config(cfg);

    ConfigParserError_t *err = NULL;

    ConfigParserStatus_t status = config_load_file(config_path, cfg, &err);

    if (status != CONFIG_OK) {
        if (err) {
          printf("Error [%d] line %li col %li: %s\n", err->code, err->line, err->column, err->message);
          destroy_parser_error(&err);
        } else {
          printf("An unknown error occurred when parsing the config\n");
        }

        destroy_app_config();
        return 1;
    }

    // Simple verification of values
    // print_app_config(cfg);
    // Clean up
    destroy_app_config();

    printf("Config loader test passed.\n");
    return 0;
}
