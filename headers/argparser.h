#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <getopt.h>

// Global argument struct
struct glob_args_t {
    /* TODO Logger daemon */
    char* log_fpath; // -l PATH
    char* cfg_fpath; // -c PATH
} glob_args;

// Parse command-line arguments
void get_cli_args(int argc, char **argv);

#endif
