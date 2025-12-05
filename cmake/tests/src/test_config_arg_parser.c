#include <stdbool.h>
#include "include/config_parser.h"

/* ____________________________ AI GENERATED _________________________ */

int main(int argc, char *argv[]) {
    // Step 1: Create schema
    StackError_t *err = NULL;
    AppConfig_t **cfg_ptr = get_app_config_handle(), *cfg = NULL;
    merge_configs(argc, argv, &err);
    int code = 0;

    if (cfg_ptr) cfg = *cfg_ptr;

    if (!cfg) {
      if (err) {
        fprintf(stderr, "Error [%d]: %s\n", err->code, err->message);
        code = err->code;

        destroy_stack_error(err);
        return code;
      }

      fprintf(stderr, "unknown error occurred while merging configs!");

      return -1;
    }

    print_app_config(cfg);
    destroy_app_config();

    return 0;
}
