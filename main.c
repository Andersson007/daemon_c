#include "headers/argparser.h"
#include "headers/control_process.h"
#include "headers/daemonize.h"
#include "headers/general.h"


int main (int argc, char **argv) {

    int exit_code = 0;

    glob_args.log_file = NULL;

    // Get command-line arguments
    get_cli_args(argc, argv);

    pid_t pid = rundaemon(0,                        // Daemon creation flags
                          control_process, NULL,    // Daemon body function and its argument
                          &exit_code,               // Pointer to a variable to receive daemon exit code
                          "/var/run/daemon.pid");   // Path to the PID-file

    switch (pid) {
        case -1:  // Low lever error. See errno for details
            perror("Cannot start daemon.");
            return EXIT_FAILURE;
            break;
        case -2:  // Daemon is already running
            fprintf(stderr, "Daemon already running.\n");
            break;
        case 0:   // Success
            return exit_code;  // Return daemon exit code
        default:
            printf("Parent: %d, Daemon: %d\n", getpid(), pid);
            break;
    }

    return EXIT_SUCCESS;
}
