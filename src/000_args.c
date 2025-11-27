#include "include/arguments.h"
#include <string.h>

/**
 * parse_args - parses the arguments passed to the program
 * @flag_schema: a hashmap of keys to the expected value types
 *    defines a typing for the flags
 * @parsed_args: a populated hashmap containing the key-value pairs
 *    of the flags
 * @err: a pointer to the error object that's populated in the event
 * of an invariant violation or failure
 * @argc: argument count
 * @argv: argument vector
 * Returns: ArgParserStatus_t - indicating status
 */
ArgParserStatus_t parse_args(
  FlagSchemaEntry_t *flag_schema,
  Argument_t **parsed_args,
  ArgParserError_t **err,
  int argc,
  char *argv[])
{
  char *token = NULL, *tmp = NULL;

  for (int i = 1; i < argc; i++)
  {
    char *arg = argv[i];

    if (arg[0] != '-') continue;

    char *flag;

    tmp = strdup(arg);

    if (arg[1] == '-') {
      flag = strtok(tmp + 2 * sizeof(char), "=");
    } else {
      flag = strtok(tmp + 1 * sizeof(char), "=");
    }

    FlagSchemaEntry_t *schema_entry = NULL;

    HASH_FIND_STR(flag_schema, flag, schema_entry);

    if (!schema_entry)
    {
      if (err) {
        *err = malloc(sizeof(ArgParserError_t));
        (*err)->code = ARG_UNKNOWN_KEY;
        snprintf((*err)->message, BUF_LEN_M,
              "Unknown argument '%s'", flag);
      }

      return ARG_UNKNOWN_KEY;
    }

    void *value = NULL;

    switch(schema_entry->type)
    {
      case ARG_TYPE_BOOL:
      {
        value = malloc(sizeof(bool));
        *(bool *)value = true;
        break;
      }
      case ARG_TYPE_INT:
      {
        token = strtok(NULL, "=");

        if (!token)
        {
          if (err) {
            *err = malloc(sizeof(ArgParserError_t));
            (*err)->code = ARG_MISSING_VALUE;
            snprintf((*err)->message, BUF_LEN_M,
                    "Missing value for argument '%s'", flag);
          }
          free(tmp);

          return ARG_MISSING_VALUE;
        }
        // ASSUMPTION: no negative values passed for numeric arguments
        if (token[0] == '-') {
          if (err) {
            *err = malloc(sizeof(ArgParserError_t));
            (*err)->code = ARG_INVALID_TYPE;
            snprintf((*err)->message, BUF_LEN_M,
                    "Invalid value for argument '%s'. no negative numbers allowed", flag);
          }
          free(tmp);

          return ARG_INVALID_TYPE;
        }

        value = malloc(sizeof(size_t));
        *(size_t *)value = (size_t)atoi(token);

        break;
      }
      case ARG_TYPE_STRING:
      {
        token = strtok(NULL, "=");
        if (!token)
        {
          if (err) {
            *err = malloc(sizeof(ArgParserError_t));
            (*err)->code = ARG_MISSING_VALUE;
            snprintf((*err)->message, BUF_LEN_M,
                    "Missing value for argument '%s'", flag);
          }
          free(tmp);

          return ARG_MISSING_VALUE;
        }
        value = strdup(token);
        break;
      }
      default:
        break;
    }

    Argument_t *parsed = malloc(sizeof(Argument_t));

    parsed->key = strdup(flag);
    parsed->value = value;
    parsed->type = schema_entry->type;
    HASH_ADD_KEYPTR(hh, *parsed_args, parsed->key, strlen(parsed->key), parsed);

    free(tmp);
  }

  return ARG_SUCCESS;
}

void destroy_parsed_argument(Argument_t *arg) {
  Argument_t *current, *tmp;

  HASH_ITER(hh, arg, current, tmp) {
    HASH_DEL(arg, current);
    if (current->key) free(current->key);
    if (current->value) free(current->value);
    free(current);
  }
}

void destroy_flag_schema(FlagSchemaEntry_t *schema) {
  FlagSchemaEntry_t *current, *tmp;

  HASH_ITER(hh, schema, current, tmp) {
    HASH_DEL(schema, current);
    free(current);
  }
}

void add_flag(FlagSchemaEntry_t **schema, const char *key, ArgType type) {
  FlagSchemaEntry_t *entry = malloc(sizeof(FlagSchemaEntry_t));

  strncpy(entry->key, key, BUF_LEN_S);
  entry->type = type;
  HASH_ADD_STR(*schema, key, entry);
}
