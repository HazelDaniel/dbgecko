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
