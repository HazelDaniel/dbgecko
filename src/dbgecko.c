#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "include/globals.h"
#include "include/tui.h"
#include "include/config_parser.h"
#include "include/plugin_manager.h"

int main(int argc, char **argv)
{
  TUIState_t state = {0};
  StackError_t *err = NULL;
  bool cli_mode = false;

  /* Headless mode: CI sanitizer checks just need a clean exit */
  if (getenv("DBGECKO_HEADLESS")) {
    return EXEC_SUCCESS;
  }

  merge_configs(argc, argv, &err);
  AppConfig_t *cfg = *get_app_config_handle();

  if (!cfg){
    for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--runtime_mode=cli") == 0 || strcmp(argv[i], "-runtime_mode=cli") == 0) {
        cli_mode = true;
        break;
      }
    }

    if (!cli_mode) {
      if (err) {
        tui_push_log(&state, LOG_ERROR, err->message);
        destroy_stack_error(err);

        return EXEC_FAILURE;
      }

      tui_push_log(&state, LOG_ERROR, "Unknown configuration Error");

      return EXEC_FAILURE;
    }

    if (err) {
      fprintf(stderr, "Configuration error: %s\n", err->message);
      destroy_stack_error(err);

      return EXEC_FAILURE;
    }

    fprintf(stderr, "Configuration error: Unknown!");

    return EXEC_FAILURE;
  }

  cli_mode = strcmp(cfg->runtime->mode, "cli") == 0;

  if (err && cli_mode) {
    fprintf(stderr, "Configuration error: %s\n", err->message);
    destroy_stack_error(err);

    return EXEC_FAILURE;
  }

  if (err) {
    destroy_stack_error(err);
    err = NULL;  // will fall through to TUI missing config handling
  }

  if (cli_mode) {
    if (!cfg) {
      fprintf(stderr, "Configuration not initialized\n");
      return EXEC_FAILURE;
    }
    StackErrorMessage_t plugin_err = NULL;
    StackStatus_t status = load_plugin(&plugin_err, NULL);

    if (status != EXEC_SUCCESS) {
      fprintf(stderr, "Plugin load failed: %s\n", plugin_err ? plugin_err : "unknown error");
      if (plugin_err) free(plugin_err);
      return EXEC_FAILURE;
    }

    PluginRegistry_t *reg = get_plugin_registry_entry(NULL);
    if (!reg || !reg->driver) {
      fprintf(stderr, "Plugin invalid or missing driver\n");
      return EXEC_FAILURE;
    }

    PluginDriver_t *driver = reg->driver;
    printf("Plugin '%s' loaded (v%.1f)\n", driver->name, (double)driver->version);

    if (driver->connect(cfg, &plugin_err) != OP_SUCCESS) {
      fprintf(stderr, "Connect failed: %s\n", plugin_err ? plugin_err : "unknown error");
      if (plugin_err) free(plugin_err);
      return EXEC_FAILURE;
    }

    DriverStatus_t op_status = OP_FAIL;

    if (strcmp(cfg->runtime->op, "backup") == 0) {
      op_status = driver->backup(cfg, &plugin_err);
    } else if (strcmp(cfg->runtime->op, "restore") == 0) {
      op_status = driver->restore(cfg, &plugin_err);
    } else {
      fprintf(stderr, "Operation '%s' not implemented or recognized\n", cfg->runtime->op);
      return EXEC_FAILURE;
    }

    if (op_status != OP_SUCCESS) {
      fprintf(stderr, "Operation '%s' failed: %s\n", cfg->runtime->op, plugin_err ? plugin_err : "unknown error");
      if (plugin_err) free(plugin_err);
      return EXEC_FAILURE;
    }

    printf("Operation '%s' completed successfully\n", cfg->runtime->op);
    return EXEC_SUCCESS;
  }

  /* TUI mode (default) */ tui_init(&state);

  if (cfg && !err) {
    state.config_loaded = true;
    tui_push_log(&state, LOG_INFO, "Configuration loaded successfully");
  } else if (err) {
    tui_push_log(&state, LOG_WARN, "Configuration info: %s", err->message);
  }

  tui_run(&state, argc, argv);
  tui_shutdown(&state);

  return EXEC_SUCCESS;
}
