#ifndef ARGUMENTS_H
#define ARGUMENTS_H
#include <stdbool.h>
#include <stdio.h>
#include "globals.h"

/*
 * ==========================================================
 * Generic Argument Registration System
 * ----------------------------------------------------------
 * This module provides a lightweight and extensible way
 * to register and manage command-line arguments (flags,
 * positional arguments, etc.) for a CLI application.
 *
 * It defines:
 *  - `ArgType` : the data type of each argument (bool, int, string, etc.)
 *  - `Flag`    : how the argument is specified (positional, short flag, long flag)
 *  - `Arguments` : metadata for a single argument definition
 *  - `ArgParser` : container/registry for all parsed arguments
 *  - `Options`   : user-defined structure representing application options
 *
 * It also defines the macro `DEFINE_ARG_FUNC` to easily create
 * strongly-typed argument registration helper functions like:
 *    arg_bool(), arg_int(), arg_string()
 *
 * Example usage:
 *    ArgParser parser;
 *    bool verbose;
 *    int port;
 *    const char *host;
 *
 *    arg_bool(&parser, "verbose", ARG_LONG_FLAG, "Enable verbose mode", &verbose, false);
 *    arg_int(&parser, "port", ARG_SHORT_FLAG, "Database port", &port, true);
 *    arg_string(&parser, "host", ARG_LONG_FLAG, "Database host", &host, true);
 *
 * ==========================================================
 */

/* ----------------------------------------------------------
 * Argument Type Enum
 * ----------------------------------------------------------
 * Defines the underlying data type of each argument.
 */
typedef enum
{
    ARG_TYPE_BOOL,
    ARG_TYPE_INT,
    ARG_TYPE_STRING,
    ARG_TYPE_FLOAT
} ArgType;

/* ----------------------------------------------------------
 * Flag Type Enum
 * ----------------------------------------------------------
 * Defines how the argument is presented in the CLI.
 *  - ARG_POSITIONAL : argument without flag, e.g. `myapp file.txt`
 *  - ARG_SHORT_FLAG : short option, e.g. `-v`
 *  - ARG_LONG_FLAG  : long option, e.g. `--verbose`
 */
typedef enum
{
    ARG_POSITIONAL,
    ARG_SHORT_FLAG,
    ARG_LONG_FLAG
} Flag;

/* ----------------------------------------------------------
 * Options Struct
 * ----------------------------------------------------------
 * Example of a user-defined structure that maps parsed
 * arguments into application configuration values.
 */
typedef struct
{
    const char *db;
    const char *dbtype;
    const char *host;
    int port;
    const char *password;
    const char *user;
    const char *db_uri;
    const char *help;
} Options;

/* ----------------------------------------------------------
 * Arguments Struct
 * ----------------------------------------------------------
 * Represents a single argument definition, including
 * its metadata, data type, target storage, and description.
 */
typedef struct
{
    const char *name;
    Flag flag;
    ArgType type;
    const char *description;
    Options *value;
    bool required;
} Arguments;

/* ----------------------------------------------------------
 * ArgParser Struct
 * ----------------------------------------------------------
 * Holds a dynamic list of registered arguments.
 * This structure is passed around during parsing.
 */
typedef struct
{
    Arguments *args;
    int arg_counts;
    int capacity;
} ArgParser;

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


// function declaration
ArgParser *register_args();
void arg_parser(ArgParser *parser, int argc, char *argv[]);
void free_args_parser(ArgParser *parser);
void append_args(ArgParser *parser, Arguments arg);
void free_args_parser(ArgParser *parser);
void destroy_parsed_argument(Argument_t *arg);
void destroy_flag_schema(FlagSchemaEntry_t *schema);
void add_flag(FlagSchemaEntry_t **schema, const char *key, ArgType type);

ArgParserStatus_t parse_args(
  FlagSchemaEntry_t *flag_schema,
  Argument_t **parsed_args,
  ArgParserError_t **err,
  int argc,
  char *argv[]);

// Macro definition
/* ----------------------------------------------------------
 * Generic Argument Registration Macro
 * ----------------------------------------------------------
 * Defines type-specific argument registration functions.
 *
 * Example:
 *    DEFINE_ARG_FUNC(bool, ARG_TYPE_BOOL)
 *    => creates:
 *       void arg_bool(ArgParser *parser, const char *name, Flag *flag,
 *                     const char *desc, void *value, bool required);
 */
#define DEFINE_ARG_FUNC(type_name, arg_type_enum)                        \
    void arg_##type_name(ArgParser *parser, const char *name, Flag flag, \
                         const char *desc, void *value, bool required)   \
    {                                                                    \
        Arguments arg = {                                                \
            .name = name,                                                \
            .flag = flag,                                                \
            .type = arg_type_enum,                                       \
            .description = desc,                                         \
            .required = required,                                        \
            .value = value};                                             \
        append_args(parser, arg);                                        \
    }

/* ----------------------------------------------------------
 * Concrete Argument Registration Functions
 * ----------------------------------------------------------
 * Automatically generated by the macro above.
 */

void arg_bool(ArgParser *, const char *, Flag, const char *, void *, bool);
void arg_int(ArgParser *, const char *, Flag, const char *, void *, bool);
void arg_string(ArgParser *, const char *, Flag, const char *, void *, bool);

/*
 * ==========================================================
 * DYNAMIC_ARRAY_APPEND Macro
 * ----------------------------------------------------------
 * Generic macro for dynamically appending an element to a
 * resizable array structure.
 *
 * Automatically doubles the array's capacity when full and
 * reallocates memory safely. Works with any array type as long
 * as your struct follows this convention:
 *
 *    struct {
 *        ElementType *items;   // pointer to the array
 *        int count;            // current number of elements
 *        int capacity;         // current allocated capacity
 *    };
 *
 * Example usage:
 *    DYN_ARRAY_APPEND(parser->args, parser->arg_counts, parser->capacity, arg);
 *
 * You can also wrap it for a specific struct:
 *    #define APPEND_ARGS(parser, arg) \
 *        DYN_ARRAY_APPEND(parser->args, parser->arg_counts, parser->capacity, arg)
 *
 * ==========================================================
 */

#define DYN_ARRAY_APPEND(array, count, capacity, value)                      \
    do                                                                       \
    {                                                                        \
        /* Check if we need to expand the array */                           \
        if ((count) >= (capacity))                                           \
        {                                                                    \
            (capacity) = (capacity) ? (capacity) * 2 : 4;                    \
            void *tmp_ptr = realloc((array), sizeof(*(array)) * (capacity)); \
            if (!tmp_ptr)                                                    \
            {                                                                \
                fprintf(stderr, "Error: memory reallocation failed.\n");     \
                free(array);                                                             \
                exit(EXIT_FAILURE);                                          \
            }                                                                \
            (array) = tmp_ptr;                                               \
        }                                                                    \
        (array)[(count)++] = (value);                                        \
    } while (0)

#endif
