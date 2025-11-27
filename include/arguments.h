#ifndef ___ARGUMENTS_H___
#define ___ARGUMENTS_H___
#include <stdbool.h>
#include <stdio.h>
#include "globals.h"

typedef enum
{
  ARG_TYPE_BOOL,
  ARG_TYPE_INT,
  ARG_TYPE_STRING,
  ARG_TYPE_FLOAT
} ArgType;

typedef enum {
  ARG_SUCCESS = 0, //order important for casting into stack error
  ARG_INVALID_TYPE,
  ARG_MISSING_VALUE,
  ARG_UNKNOWN_KEY,
} ArgParserStatus_t;

typedef struct FlagSchemaEntry {
  char              key[BUF_LEN_S];
  ArgType           type;
  UT_hash_handle    hh;   // makes this struct hashable by uthash
} FlagSchemaEntry_t;

typedef struct ArgParserError {
  ArgParserStatus_t     code;
  char                  message[BUF_LEN_M];
} ArgParserError_t;

typedef struct Argument {
  char              *key;
  void              *value;
  ArgType           type;
  UT_hash_handle    hh; // makes this struct hashable by uthash
} Argument_t;


void destroy_parsed_argument(Argument_t *arg);
void destroy_flag_schema(FlagSchemaEntry_t *schema);
void add_flag(FlagSchemaEntry_t **schema, const char *key, ArgType type);

ArgParserStatus_t parse_args(
  FlagSchemaEntry_t *flag_schema,
  Argument_t **parsed_args,
  ArgParserError_t **err,
  int argc,
  char *argv[]);

#endif /* ___ARGUMENTS_H___ */
