#define _POSIX_C_SOURCE 1

#include <glib.h>
#include <libconfig.h>
#include <sys/types.h>

#include "headers/argparser.h"
#include "headers/backend.h"
#include "headers/control_process.h"
#include "headers/daemonize.h"
#include "headers/general.h"
#include "headers/logger.h"

static inline pid_t get_b_pid(GList* item);

static inline int get_b_type(GList* item);

static void terminate_service_backends(GQueue* b_list);

static logger_params* make_logger_params(GQueue* log_queue, char* log_fpath);


// Control process body
int control_process(void *udata) {
    char msg[LOG_REC_BUF_LEN];  // Log msg buffer

    // Defaults
    int exit_code = EXIT_SUCCESS;

    glob_args.log_fpath = DEFAULT_LOG_PATH;
    glob_args.cfg_fpath = DEFAULT_CNF_PATH;

    // Get params from main
    cproc_params* params = (cproc_params*) udata;

    // Get command-line arguments
    get_cli_args(params->argc, params->argv);

    // Initialize backend list
    GQueue* b_list = g_queue_new();
    g_queue_push_tail(b_list, make_backend(getpid(), CONTROL_PROCESS));

    GList* cnt_proc = g_queue_peek_nth_link(b_list, 0);

    // Initialize log shared structure
    GQueue* log_queue = g_queue_new();
    to_log_queue(log_queue, INF,
                 "Control process initialized (pid %d)\n", getpid());

    // Open the system log
    openlog("daemon Control process", LOG_NDELAY, LOG_DAEMON);

    // Greeting
    syslog(LOG_INFO, "Control process started. PID: %d, TYPE: %d",
           get_b_pid(cnt_proc), get_b_type(cnt_proc));

    /* Reading config related tasks */
    config_t cfg;
    config_setting_t* setting;
    config_init(&cfg);

    /* DEBUG */
    syslog(LOG_INFO, "Control process: try to read conf file '%s'",
           glob_args.cfg_fpath);
    /* ***** */

    if (!config_read_file(&cfg, glob_args.cfg_fpath)) {
        syslog(LOG_ERR, "Control process: reading of cfg file failed %s:%d - %s",
               config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(EXIT_FAILURE);
    }
    /* DEBUG */
    syslog(LOG_INFO, "Control process: cfg file is '%s'", glob_args.cfg_fpath);
    /* ***** */
    setting = config_lookup(&cfg, "log_file_path");
    if (setting != NULL && !strcmp(glob_args.log_fpath, DEFAULT_LOG_PATH)) {
        glob_args.log_fpath = (char* ) config_setting_get_string(setting);
    }
    /* Reading config related tasks */

    // Logger related actions
    FILE* log_fp = fopen(glob_args.log_fpath, "a+");
    if (!log_fp) {
        syslog(LOG_ERR, "Control process: could not open the log file %s",
               glob_args.log_fpath);
        exit(1);
    }
    else {
        /* DEBUG */
        fprintf(log_fp, "Control process: 'hello'\n");
        fflush(log_fp);
        /**/
        fclose(log_fp);
    }

    logger_params* log_params = make_logger_params(log_queue, glob_args.log_fpath);

    /* Logger process related tasks */ 
    // Start Logger process
    pid_t pid = rundaemon(0,                            // Daemon creation flags
                          logger, (void*) log_params,   // Daemon body function and its argument
                          &exit_code,                   // Pointer to a variable to receive daemon exit code
                          "/var/run/logger.pid");       // Path to the PID-file

    // Add Logger process's pid and type to process list
    g_queue_push_tail(b_list, make_backend(pid, LOGGER_PROCESS));
    /* End of Logger process related tasks */


    /* Signal handling prep related tasks */
    int sfd = -1;
    sigset_t mask;
    struct signalfd_siginfo si;

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
        perror("Control process: signalfd failed");
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        closelog();
        return EXIT_FAILURE;
    }
    /* End of signal handling prep tasks */

    // Timeout for waiting signals by select() func
    struct timeval signal_timeo = { .tv_sec = SIG_WAIT_TIMEO, .tv_usec = 0 };

    // The control process loop
    int need_exit = 0;
    while (!need_exit) {
        /* DEBUG */
        syslog(LOG_INFO, "CP: in loop, pid %d, ppid %d", getpid(), getppid());
        sleep(5);
        /*********/

        int result;
        fd_set readset;

        // Add the signal file descriptor to set
        FD_ZERO(&readset);
        FD_SET(sfd, &readset);
        // One could add more file descriptors here
        // and handle them accordingly if one wants to build a server
        // using event-driven approach

        // Wait for the data in the signal file descriptor
        result = select(FD_SETSIZE, &readset, NULL, NULL, &signal_timeo);
        if (result == -1) {
            syslog(LOG_ERR, "Control process: fatal error during select() call.");
            // Low level error
            exit_code = EXIT_FAILURE;
            break;
        }

        // Read the data from the signal handler file descriptor
        if (FD_ISSET(sfd, &readset) && read(sfd, &si, sizeof(si)) > 0) {
            // Handle the signals
            switch (si.ssi_signo) {
                case SIGTERM:   // Stop the daemon
                    syslog(LOG_INFO, "Control process: got SIGTERM signal. "
                                     "Terminate backends and stop daemon...");
                    terminate_service_backends(b_list);
                    need_exit = 1;
                    break;
                case SIGHUP:    // Reload the configuration
                    syslog(LOG_INFO, "Control process: got SIGHUP signal.");
                    break;
                default:
                    syslog(LOG_WARNING,
                           "Control process: got unexpected signal (number: %d).",
                           si.ssi_signo);
                    break;
            }
        }
    }

    // Clean up
    g_queue_free_full(b_list, g_free);      // Backend list
    g_queue_free_full(log_queue, g_free);   // Log message queue
    free(log_params);                       // Log params
    g_free(params);                         // Control proc params
    config_destroy(&cfg);                   // Configuration struct

    // Close the signal file descriptor
    close(sfd);
    // Remove the sighal handlers
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // Write an exit code to the system log
    syslog(LOG_INFO, "Control process: stopped with status code %d.", exit_code);
    // Close the system log
    closelog();

    //return exit_code;
    exit(exit_code);
}


void terminate_service_backends(GQueue* b_list) {

    GList* b_proc = NULL;
    int k_rc = 0;

    while (1) {
        b_proc = g_queue_peek_tail_link(b_list);

        if (get_b_type(b_proc) == CONTROL_PROCESS || b_proc == NULL) {
            /* DEBUG */
            syslog(LOG_INFO, "CP: DONT kill %d, it is CP itself %d",
                   get_b_pid(b_proc), get_b_type(b_proc));
            /*********/
            break;
        }

        /* DEBUG */
        syslog(LOG_INFO, "CP: goint to kill %d", get_b_pid(b_proc));
        /*********/

        // Send SIGTERM to the backend and
        // remove its structure from the backend list
        k_rc = kill(get_b_pid(b_proc), SIGTERM);
        if (k_rc == EXIT_SUCCESS) {
            syslog(LOG_INFO, "CP: killed %d", get_b_pid(b_proc));
        }
        else {
            syslog(LOG_INFO, "CP: cannot kill %d, rcode %d",
                   get_b_pid(b_proc), k_rc);
        }
        g_free(b_proc->data);
        g_queue_pop_tail(b_list);
    }
}


// Return ptr to struct backend process representation
backend_node* make_backend(pid_t b_pid, unsigned b_type) {
    backend_node* b = g_new(backend_node, 1);
    b->b_pid = b_pid;
    b->b_type = b_type;
    return b;
}


// Return ptr to struct for logger params
static logger_params* make_logger_params(GQueue* log_queue, char* log_fpath) {
    logger_params* p = g_new(logger_params, 1);
    p->log_queue = log_queue;
    p->log_fpath = log_fpath;
    return p;
}


// Get backend process pid from backend list item
static inline pid_t get_b_pid(GList* item) {
    return ((backend_node*)item->data)->b_pid;
}


// Get backend process type from backend list item
static inline int get_b_type(GList* item) {
    return ((backend_node*)item->data)->b_type;
}
