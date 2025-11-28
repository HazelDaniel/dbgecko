#include <stdbool.h>
#include "include/config_parser.h"
#include "include/plugin_manager.h"

/* ____________________________ AI GENERATED _________________________ */

int main(int argc, char *argv[]) {
    // Step 1: Create schema
    StackError_t *err = NULL;
    AppConfig_t **cfg_ptr = get_app_config_handle(), *cfg = NULL;
    PluginRegistry_t **reg_ptr = get_plugin_registry_handle();
    merge_configs(argc, argv, &err);
    int code = 0;
    char *err_message = NULL;
    StackStatus_t status = EXEC_SUCCESS;

    if (cfg_ptr) cfg = *cfg_ptr;

    if (!cfg) {
      if (err) {
        printf("Error [%d]: %s\n", err->code, err->message);
        code = err->code;

        destroy_stack_error(err);
        return code;
      }

      printf("unknown error occurred while merging configs!");

      return EXEC_FAILURE;
    }

    status = load_plugin(&err_message, cfg->db->type);

    if (status != EXEC_SUCCESS) {
      if (err_message) {
        printf("Error: %s\n", err_message);
        free(err_message);
      } else {
        printf("Unknown error occurred while loading the plugin\n");
      }

      destroy_app_config();

      return err_message ? EXEC_SUCCESS : EXEC_FAILURE;
    }

    destroy_app_config();
    status = destroy_plugin_registry(&err_message);

    if (status != EXEC_SUCCESS) {
      if (err_message) {
        printf("Error: %s\n", err_message);
        free(err_message);
      }
    }

    return status;
}
