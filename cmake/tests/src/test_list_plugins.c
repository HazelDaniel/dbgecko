#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "include/globals.h"
#include "include/plugin_manager.h"
#include "lib/include/slre.h"

/* --------------AI GENERATED--------------- */

void print_top_level_files(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char fullpath[BUF_LEN_L];
    struct stat st;

    // Pattern: files starting with 'test' and containing 'config'
    const char *pattern = POSTGRES_PLUGIN_REGEX;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (stat(fullpath, &st) != 0)
            continue;

        if (S_ISREG(st.st_mode)) {
            // Skip files matching pattern
            if (slre_match(pattern, entry->d_name, strlen(entry->d_name), NULL, 0, 0) != strlen(entry->d_name))
              printf("(not valid)\t");

            // Print all other regular files
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir);
    destroy_app_config();
}

int main(int argc, char *argv[]) {
  StackError_t *err = NULL;
  AppConfig_t **cfg_ptr = get_app_config_handle(), *cfg = NULL;
  merge_configs(argc, argv, &err);
  size_t status = 0;

  cfg = *cfg_ptr;

  if (!cfg) {
    if (err) {
      fprintf(stderr, "Error [%d]: %s\n", err->code, err->message);

      destroy_stack_error(err);
      status = err->code;
      return status;
    }

    fprintf(stderr, "unknown error occurred while merging configs!");

    return status;
  }

  print_top_level_files(cfg->plugin->dir);

  destroy_app_config();

  return 0;
}
