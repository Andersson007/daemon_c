#include "headers/argparser.h"


// Get and set command-line arguments.
// If you need new one, add it to the glob_args struct in argparser.h,
// then add to the switch block below
void get_cli_args(int argc, char **argv) {

    int opt = 0;

    // Allowable command-line arguments
    const char *opt_string = "l:";

    opt = getopt(argc, argv, opt_string);
    while (opt != -1) {
        switch(opt) {
            case 'l':
                glob_args.log_file = optarg;
                break;
            default:
                break;
        }

        opt = getopt(argc, argv, opt_string);
    }
}
