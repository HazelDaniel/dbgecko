#include "include/arguments.h"
#include <stdbool.h>

int main(int argc, char **argv)
{
    Arguments *args;
    ArgParser *parser = register_args();
    Options opt;

    // register flags
    arg_string(parser, "type", ARG_LONG_FLAG, "provide database type for to perform backup on", &opt.dbtype, true);
    arg_bool(parser, "help", ARG_LONG_FLAG, "provide help about dbgecko", &opt.help, false);
    arg_int(parser, "p", ARG_SHORT_FLAG, "database port provided", &opt.port, true);
    arg_parser(parser, argc, argv);

    // printf("database type: %s\n", opt.dbtype);
    // printf("help flag: %s\n", opt.help);
    // printf("port: provided %d", opt.port);

    free_args_parser(parser);
    return 0;
}
