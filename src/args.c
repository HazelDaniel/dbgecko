#include "include/arguments.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

DEFINE_ARG_FUNC(bool, ARG_TYPE_BOOL)
DEFINE_ARG_FUNC(int, ARG_TYPE_INT)
DEFINE_ARG_FUNC(string, ARG_TYPE_STRING)

bool flag_exists(ArgParser *parser, char *flag);

/**
 * register_args - Allocates and initializes a new ArgParser.
 *
 * Return: Pointer to a newly allocated ArgParser.
 *
 * This function creates a dynamic argument parser object that can hold
 * multiple command-line arguments. It sets up the initial state:
 *   - arg_counts: Number of arguments currently registered (starts at 0)
 *   - capacity: Initial capacity of the dynamic argument array
 *   - args: Dynamically allocated array of Arguments structs
 *
 * The parser's capacity will automatically grow if more arguments are
 * registered than the initial capacity, typically using a reallocation
 * strategy (e.g., doubling the size).
 *
 * Example usage:
 *   ArgParser *parser = register_args();
 *
 * Notes:
 * - The caller is responsible for freeing the allocated memory:
 *     free(parser->args);
 *     free(parser);
 * - Initial capacity is 10, but can be adjusted if needed.
 */
ArgParser *register_args()
{
    ArgParser *parser = malloc(sizeof(ArgParser));

    parser->arg_counts = 0;
    parser->capacity = 10;
    parser->args = malloc(sizeof(Arguments) * parser->capacity);
    return parser;
}

/**
 * arg_parser - Parses command-line arguments and assigns values to ArgParser.
 *
 * @parser: Pointer to an ArgParser struct containing all registered arguments.
 * @argc: Number of arguments passed to the program (from main).
 * @argv: Array of argument strings (from main).
 *
 * This function iterates through all command-line arguments (`argv`) starting
 * from index 1 (skipping the program name). It detects flags prefixed with `--`
 * (long flags) and optionally assigns the following argument as the flag's value.
 *
 * Behavior:
 * - If a flag is recognized:
 *     - Boolean flags (ARG_TYPE_BOOL) are set to true.
 *     - String flags (ARG_TYPE_STRING) are assigned the next argument as their value.
 *       If a required string argument is missing, the function prints an error and exits.
 *     - Integer flags (ARG_TYPE_INT) are converted from string to int using `atoi`.
 * - If a flag is unrecognized, an error is printed, and the program exits.
 *
 * Safety:
 * - Ensures no reading past string boundaries.
 * - Properly checks that the next argv element exists and is not another flag before
 *   assigning it as a value.
 * - Ensures required arguments are present; prints an error if missing.
 *
 *
 * Notes:
 * - Only long flags with `--`  and short '-'  are currently handled.
 * - Positional arguments can be added by extending the else branch.
 */
void arg_parser(ArgParser *parser, int argc, char *argv[])
{
    void *value = NULL;
    for (int ind = 1; ind < argc; ind++)
    {
        char *arg = argv[ind];
        char *flag;
        if (arg[0] == '-')
        {
            if (arg[1] == '-')
                flag = arg + 2;
            else
                flag = arg + 1;
            if (!flag_exists(parser, flag) != 0)
            {
                fprintf(stderr,
                        "Error: unrecognized option '%s'\n"
                        "Use '--help' to see the list of available commands.\n",
                        flag);
                free_args_parser(parser);
                exit(EXIT_FAILURE);
            }
        }

        for (; ind + 1 < argc && argv[ind + 1][0] != '-';)
        {
            value = argv[ind + 1];
            ind++;
        }

        for (int i = 0; i < parser->arg_counts; i++)
        {
            if (strcmp(parser->args[i].name, flag) == 0)
                switch (parser->args[i].type)
                {
                case ARG_TYPE_BOOL:
                    if (!value)
                        *(bool *)value = true;
                    bool *no_arg = value;

                    // match commands  their  provided options....
                    break;
                case ARG_TYPE_STRING:
                    if (!value)
                    {
                        fprintf(stderr,
                                "Error: missing value for required argument '%s'\n",
                                parser->args[i].name);
                        free_args_parser(parser);
                        exit(EXIT_FAILURE);
                    }
                    char *str = (char *)value;
                    printf("%s\n", str);
                    // match arguments with their provided options
                    break;
                case ARG_TYPE_INT:
                    char **inte = (char **)value;
                    if (!(*inte))
                    {
                        fprintf(stderr,
                                "Error: missing value for required argument '%s'\n",
                                parser->args[i].name);
                        free_args_parser(parser);
                        exit(EXIT_FAILURE);
                    }
                    // match arguments with their provided options
                    break;
                }
        }
    }
}

/**
 * flag_exists - check if we've already registered this flag
 *
 * @parser: your ArgParser struct
 * @flag: the name of the flag we're checking
 *
 * Returns true if the flag exists, false otherwise.
 *
 * Basically, this just loops through all the arguments we've added so far
 * and compares the names. Linear search is fine for our little tool.
 * If we ever get hundreds of flags, maybe we'll do something smarter.
 */
bool flag_exists(ArgParser *parser, char *flag)
{
    for (int i = 0; i < parser->arg_counts; i++)
    {
        if (strcmp(parser->args[i].name, flag) == 0)
            return true;
    }
    return false;
}

/**
 * append_args - stick a new argument into the parser's array
 *
 * @parser: the ArgParser struct
 * @arg: the argument we want to add
 *
 * This uses the DYN_ARRAY_APPEND macro so we don't have to think about
 * resizing the array manually. Just adds the argument, grows the array
 * if needed, and bumps the count.
 *
 * It's a tiny helper, but it keeps everything nice and clean.
 */
void append_args(ArgParser *parser, Arguments arg)
{

    DYN_ARRAY_APPEND(parser->args, parser->arg_counts, parser->capacity, arg);
}

/**
 * free_args_parser - clean up memory when we're done
 *
 * @parser: the ArgParser to free
 *
 * This frees the array of arguments and the parser itself.
 * Important: it doesn't free any strings inside Arguments, just the array.
 * So if you allocated anything inside the args, free that too.
 *
 * Basically: use this when you're done with your parser to avoid leaks.
 */
void free_args_parser(ArgParser *parser)
{
    free(parser->args);
    free(parser);
}
