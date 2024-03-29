#define _POSIX_C_SOURCE 1

#include <glib.h>
#include "headers/argparser.h"
#include "headers/control_process.h"
#include "headers/daemonize.h"
#include "headers/general.h"

static cproc_params* make_cproc_params(int argc, char **argv);


int main (int argc, char **argv) {

    int exit_code = 0;

    cproc_params* params = make_cproc_params(argc, argv);

    pid_t pid = rundaemon(0,                        // Daemon creation flags
                          control_process,          // Daemon body function
                          (void*) params,           // and its argument
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


// Return ptr to struct for logger params
static cproc_params* make_cproc_params(int argc, char** argv) {
    cproc_params* p = g_new(cproc_params, 1);
    p->argc = argc;
    p->argv = argv;
    return p;
}
