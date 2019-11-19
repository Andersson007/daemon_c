#define _POSIX_C_SOURCE 1

#include <glib.h>
#include <sys/types.h>
#include "headers/backend.h"
#include "headers/control_process.h"
#include "headers/daemonize.h"
#include "headers/general.h"
#include "headers/logger.h"

inline pid_t get_b_pid(GList* item);

inline int get_b_type(GList* item);

// Return ptr to struct backend process representation
backend_node* make_backend(pid_t b_pid, unsigned b_type) {
    backend_node* b = g_new(backend_node, 1);
    b->b_pid = b_pid;
    b->b_type = b_type;
    return b;
}


// Return ptr to struct for log message
log_record* make_lrec(char* rec) {
    log_record* r = g_new(log_record, 1);
    r->rec = rec;
    return r;
}


// Return ptr to struct for logger params
logger_params* make_logger_params(GQueue* log_queue, char* log_fpath) {
    logger_params* p = g_new(logger_params, 1);
    p->log_queue = log_queue;
    p->log_fpath = log_fpath;
    return p;
}


// Get backend process pid from backend list item
inline pid_t get_b_pid(GList* item) {
    return ((backend_node*)item->data)->b_pid;
}


// Get backend process type from backend list item
inline int get_b_type(GList* item) {
    return ((backend_node*)item->data)->b_type;
}


// Control process body
int control_process(void *udata) {

    int exit_code = EXIT_SUCCESS;

    int sfd = -1;
    sigset_t mask;
    struct signalfd_siginfo si;

    // Initialize backend list
    GQueue* b_list = g_queue_new();
    g_queue_push_tail(b_list, make_backend(getpid(), CONTROL_PROCESS));

    GList* cnt_proc = g_queue_peek_nth_link(b_list, 0);

    // Initialize log shared structure
    GQueue* log_queue = g_queue_new();
    g_queue_push_tail(log_queue, "control process initialized\n");

    // Open the system log
    openlog(PROGNAME, LOG_NDELAY, LOG_DAEMON);

    // Greeting
    syslog(LOG_INFO, "control proc started. PID: %d, TYPE: %d",
           get_b_pid(cnt_proc), get_b_type(cnt_proc));

    // Logger related actions
    char* log_fpath = DEFAULT_LOG_PATH;
    FILE* log_fp = fopen(log_fpath, "a+");
    if (!log_fp) {
        syslog(LOG_ERR, "could not open the log file %s", log_fpath);
        exit(1);
    }
    else {
/* DEBUG */
        fprintf(log_fp, "control process 'hello'\n");
        fflush(log_fp);
/**/
        fclose(log_fp);
    }

    logger_params* log_params = make_logger_params(log_queue, log_fpath);

    /* Logger process related tasks */ 
    // Start Logger process
    pid_t pid = rundaemon(0,                            // Daemon creation flags
                          logger, (void*) log_params,   // Daemon body function and its argument
                          &exit_code,                   // Pointer to a variable to receive daemon exit code
                          "/var/run/logger.pid");       // Path to the PID-file

    // Add Logger process's pid and type to process list
    g_queue_push_tail(b_list, make_backend(pid, LOGGER_PROCESS));
    /* End of Logger process related tasks */

    // Create a file descriptor for signal handling
    sigemptyset(&mask);

    // Handle the following signals
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);

    // Block the signals so that they aren't handled
    // according to their default dispositions
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        closelog();
        return EXIT_FAILURE;
    }

    sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd failed");
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        closelog();
        return EXIT_FAILURE;
    }

    // The control process loop
    int need_exit = 0;
    while (!need_exit) {
        int result;
        fd_set readset;

        // Add the signal file descriptor to set
        FD_ZERO(&readset);
        FD_SET(sfd, &readset);
        // One could add more file descriptors here
        // and handle them accordingly if one wants to build a server
        // using event-driven approach

        // Wait for the data in the signal file descriptor
        result = select(FD_SETSIZE, &readset, NULL, NULL, NULL);
        if (result == -1) {
            syslog(LOG_ERR, "Fatal error during select() call.");
            // Low level error
            exit_code = EXIT_FAILURE;
            break;
        }

        // Read the data from the signal handler file descriptor
        if (FD_ISSET(sfd, &readset) && read(sfd, &si, sizeof(si)) > 0) {
            // Handle the signals
            switch (si.ssi_signo) {
                case SIGTERM:   // Stop the daemon
                    syslog(LOG_INFO, "Got SIGTERM signal. Stopping daemon...");
                    need_exit = 1;
                    break;
                case SIGHUP:    // Reload the configuration
                    syslog(LOG_INFO, "Got SIGHUP signal.");
                    break;
                default:
                    syslog(LOG_WARNING, "Got unexpected signal (number: %d).", si.ssi_signo);
                    break;
            }
        }
    }

    // Clean up
    g_queue_free(b_list);       // Backend list
    g_queue_free(log_queue);    // Log message queue
    free(log_params);           // Log params

    // Close the signal file descriptor
    close(sfd);
    // Remove the sighal handlers
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // Write an exit code to the system log
    syslog(LOG_INFO, "Control process stopped with status code %d.", exit_code);
    // Close the system log
    closelog();

    return exit_code;
}
