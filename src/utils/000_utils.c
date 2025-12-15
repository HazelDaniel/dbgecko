#include <string.h>
#include <stdarg.h>
#include "include/globals.h"
#include "include/plugin_manager.h"


StackError_t *___unsafe_to_stack_error___(void *err) {
  StackError_t *dst = NULL;

  if (!err) {
    return NULL;
  }

  /*
     This avoids blindly casting to StackError_t when the caller
     may be passing some other error struct with the same fields. */
  typedef struct {
    int   code;
    char  message[BUF_LEN_M];
  } GenericErr;

  GenericErr *src = (GenericErr *)err;

  dst = create_stack_error();

  if (!dst) {
    return NULL;
  }

  if (src->code != EXEC_SUCCESS) {
    dst->code = EXEC_FAILURE;
  } else {
    dst->code = (StackStatus_t)src->code;
  }

  strncpy(dst->message, src->message, BUF_LEN_M - 1);
  dst->message[BUF_LEN_M - 1] = '\0';

  return dst;
}

DriverStatus_t to_driver_status(size_t s) {
  return (DriverStatus_t)s;
}

StackError_t *create_stack_error() {
  StackError_t *err = malloc(sizeof(StackError_t));
  err->code = EXEC_FAILURE;

  return err;
}

void destroy_stack_error(StackError_t *err) {
  if (!err) return;
  free(err);
}

/**
 * join_path - Utility to join file path name (linux style) by concatenating the base
 * directory path + relative path into an output path
 * @base: base directory
 * @rel: relative path
 * @out: output
 * @outlen: length of output
 * Returns: status
 */
int join_path(const char *base, const char *rel, char *out, size_t outlen) {
  size_t bl = 0;

  if (!base || !rel || !out) return -1;
  if (rel[0] == '/') {
    // treat rel as absolute; copy directly but ensure fits
    if (strlen(rel) >= outlen) return -1;
    strncpy(out, rel, outlen);
    out[outlen-1] = '\0';
    return 0;
  }

  bl = strlen(base);

  if (bl == 0) {
    if (strlen(rel) >= outlen) return -1;
    strncpy(out, rel, outlen);
    out[outlen-1] = '\0';
    return 0;
  }

  if (base[bl-1] == '/') {
    if (snprintf(out, outlen, "%s%s", base, rel) >= (int)outlen) return -1;
  } else {
    if (snprintf(out, outlen, "%s/%s", base, rel) >= (int)outlen) return -1;
  }

  return 0;
}

void set_err(const char **err, size_t size, const char *fmt, ...) {
  if (!err) return;
  if (!*err) *err = malloc(size * sizeof(char));
  if (!err) return;

  va_list ap;

  va_start(ap, fmt);
  vsnprintf((char *)*err, size, fmt, ap);
  va_end(ap);
}

/**
 * read_file_contents - read a file's content (using standard functions) given an absolute path
 * @path: the char pointer whose content is the absolute path
 * Return: the contents on success, NULL on failure
 *
 * ~Note~:
 *        Not suitable for extremely large files (long / single allocation)
 *        Assumes a regular file (not a pipe, procfs, etc.).
 */
char *read_file_contents(const char *path) {
  FILE *f = fopen(path, "rb");
  char *buffer = NULL;
  size_t read = 0;
  ssize_t size;

  if (!f) {
    return NULL;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }

  size = ftell(f);

  if (size < 0 || size >= MAX_BUF_SIZE) {
    fclose(f);
    return NULL;
  }

  rewind(f);
  buffer = malloc((size_t)size + 1);

  if (!buffer) {
    fclose(f);
    return NULL;
  }

  read = fread(buffer, 1, (size_t)size, f);
  fclose(f);

  if (read != (size_t)size) {
    free(buffer);
    return NULL;
  }

  buffer[size] = '\0';

  return buffer;
}
