#include <stdio.h>
#include <stdlib.h>

/* Your framework headers */
#include <stdbool.h>
#include "include/config_parser.h"
#include "include/plugin_manager.h"
#include "include/globals.h"


/* ____________________________ AI GENERATED _________________________ */

/* ---- TEST HARNESS ---- */

int main(int argc, char **argv)
{
    /*
     * Expected CLI format:
     *   ./test_pg_restore --db=postgres --storage=file --storage-path=/tmp/test-restore
     */

    StackError_t *err = NULL;
    char *err_message = NULL;
    int exit_code = 0;
    StackStatus_t status = EXEC_SUCCESS;

    /* Step 1: merge configs */
    merge_configs(argc, argv, &err);

    destroy_stack_error(err);

    /* Step 2: obtain final config handle */
    AppConfig_t **cfg_ptr = get_app_config_handle();
    if (!cfg_ptr || !*cfg_ptr) {
        printf("No config returned after merge.\n");
        // destroy_stack_error(err);
        return 1;
    }

    AppConfig_t *cfg = *cfg_ptr;

    status = load_plugin(&err_message, cfg->db->type);

    if (status != EXEC_SUCCESS) {
      if (err_message) {
        printf("Error: %s\n", err_message);

        destroy_plugin_registry(&err_message);
        destroy_app_config();
      } else {
        printf("Unknown error occurred while loading the plugin\n");
        destroy_plugin_registry(&err_message);
        destroy_app_config();
      }

      status =  err_message ? EXEC_SUCCESS : EXEC_FAILURE;
      if (err_message) free(err_message);

      return status;
    }

    /* Step 3: resolve plugin registry */
    PluginRegistry_t **reg_ptr = get_plugin_registry_handle();
    if (!reg_ptr || !*reg_ptr) {
        printf("Plugin registry not found.\n");

        destroy_plugin_registry(&err_message);
        destroy_app_config();
        if (err_message) free(err_message);
        return 1;
    }


    /* Step 4: Obtain plugin driver instance */
    PluginRegistry_t *reg = *reg_ptr;
    PluginDriver_t *driver = reg->driver;
    if (!driver || !driver->restore) {
        printf("Failed to acquire plugin driver or restore function.\n");

        destroy_plugin_registry(&err_message);
        destroy_app_config();
        if (err_message)free(err_message);

        return 1;
    }

    /* Step 5: execute pg_restore */
    DriverErrMessage_t derr = NULL;
    DriverStatus_t dstatus = driver->restore(cfg, &derr);

    if (dstatus != OP_SUCCESS) {
        printf("restore failed: %s\n", derr ? derr : "unknown");
        if (derr) free(derr);

        destroy_plugin_registry(&err_message);
        destroy_app_config();
        if (err_message)free(err_message);

        return 1;
    }

    destroy_plugin_registry(&err_message);
    destroy_app_config();


    printf("pg_restore test PASSED.\n");

    return 0;
}
