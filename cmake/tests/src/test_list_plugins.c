#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "include/globals.h"
#include "include/plugin_manager.h"
#include <pcre2.h>

/* --------------AI GENERATED--------------- */

int get_captures() {
    const char *subject = "User: Alice Age: 30 Country: Nigeria";
    const char *pattern = "^User: ([a-zA-Z0-9]+) Age: ([0-9]+) Country: ([a-zA-Z]+)$";

    int errcode;
    PCRE2_SIZE erroff;

    pcre2_code *re = pcre2_compile(
        (PCRE2_SPTR)pattern,
        PCRE2_ZERO_TERMINATED,
        0,
        &errcode,
        &erroff,
        NULL
    );

    if (!re) {
        printf("Compile error at offset %zu\n", (size_t)erroff);
        return -1;
    }

    // Allocate match data
    pcre2_match_data *md =
        pcre2_match_data_create_from_pattern(re, NULL);

    // Match
    int rc = pcre2_match(
        re,
        (PCRE2_SPTR)subject,
        strlen(subject),
        0,          // start offset
        0,          // options
        md,
        NULL
    );

    if (rc < 0) {
        printf("No match.\n");
        pcre2_code_free(re);
        pcre2_match_data_free(md);
        return 0;
    }

    // Captures are in the ovector
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);

    // rc = number of captured substrings INCLUDING group 0
    // group 1..3 exist for your pattern
    for (int group = 1; group <= 3; group++) {
        size_t start = ov[2 * group];
        size_t end   = ov[2 * group + 1];
        printf("Capture %d: %.*s\n",
               group,
               (int)(end - start),
               subject + start);
    }

    pcre2_code_free(re);
    pcre2_match_data_free(md);
    return 0;
}


void print_top_level_files(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char fullpath[BUF_LEN_L];
    struct stat st;

    // PCRE2 setup
    int errornumber;
    PCRE2_SIZE erroroffset;
    pcre2_code *re = pcre2_compile(
        (PCRE2_SPTR)POSTGRES_PLUGIN_REGEX,
        PCRE2_ZERO_TERMINATED,
        0,
        &errornumber,
        &erroroffset,
        NULL
    );

    if (!re) {
        PCRE2_UCHAR buffer[BUF_LEN_S];
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", erroroffset, (char *)buffer);
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (stat(fullpath, &st) != 0)
            continue;

        if (S_ISREG(st.st_mode)) {
            // Create a match data block
            pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

            int rc = pcre2_match(
                re,                              // the compiled pattern
                (PCRE2_SPTR)entry->d_name,       // subject string
                strlen(entry->d_name),           // length of subject
                0,                               // start at offset 0
                0,                               // default options
                match_data,                      // match data block
                NULL
            );

            if (rc < 0) {
                // No match
                printf("(not valid)\t");
            }

            printf("%s\n", entry->d_name);

            pcre2_match_data_free(match_data);
        }
    }

    pcre2_code_free(re);
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
  get_captures();

  destroy_app_config();

  return 0;
}
