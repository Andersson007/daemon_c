#define _POSIX_C_SOURCE 1
#include "headers/backend.h"
#include "headers/general.h"
#include "headers/logger.h"

pthread_mutex_t lq_mtx;
pthread_mutexattr_t attrmutex;

inline char* get_log_rec(GList* item);

static void init_log_queue_mutex(void);

static FILE* open_log(char* log_fpath);

// Get backend process type from backend list item
inline char* get_log_rec(GList* item) {
    return ((log_record*)item->data)->rec;
}


int logger(void* udata) {
    logger_params* log_params = (logger_params*) udata;

    int exit_code = EXIT_SUCCESS;

    int sfd = -1;
    sigset_t mask;
    struct signalfd_siginfo si;

    // Open the system log
    openlog(PROGNAME, LOG_NDELAY, LOG_DAEMON);

    // Greeting by syslog
    syslog(LOG_INFO, "Logger process started. PID: %d, TYPE: %d",
           getpid(), LOGGER_PROCESS);

    // Initialize 
    init_log_queue_mutex();

    // Open log file in "a+" mode
    FILE* log_fp = open_log(log_params->log_fpath);

    // Push greeting to log queue
    to_log_queue(log_params->log_queue, "Logger process initialized\n");

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
        perror("Logger process: signalfd failed");
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        closelog();
        return EXIT_FAILURE;
    }

    // Timeout for waiting signals by select() func
    struct timeval signal_timeo = { .tv_sec = SIG_WAIT_TIMEO, .tv_usec = 0 };

    // The control process loop
    int need_exit = 0;
    while (!need_exit) {

        // Logger process job
        handle_log_queue(log_params->log_queue, log_fp);
        //

        int result;
        fd_set readset;

        // Add the signal file descriptor to set
        FD_ZERO(&readset);
        FD_SET(sfd, &readset);
        // One could add more file descriptors here
        // and handle them accordingly if one wants to build a server
        // using event-driven approach

        // Wait for the data in the signal file descriptor
        //result = select(FD_SETSIZE, &readset, NULL, NULL, NULL);
        result = select(FD_SETSIZE, &readset, NULL, NULL, &signal_timeo);
        if (result == -1) {
            syslog(LOG_ERR, "Logger process: fatal error during select() call.");
            // Low level error
            exit_code = EXIT_FAILURE;
            break;
        }

        // Read the data from the signal handler file descriptor
        if (FD_ISSET(sfd, &readset) && read(sfd, &si, sizeof(si)) > 0) {
            // Handle the signals
            switch (si.ssi_signo) {
                case SIGTERM:   // Stop the daemon
                    syslog(LOG_INFO, "Logger process: got SIGTERM signal. Stopping daemon...");
                    need_exit = 1;
                    break;
                case SIGHUP:    // Reload the configuration
                    syslog(LOG_INFO, "Logger process: got SIGHUP signal.");
                    break;
                default:
                    syslog(LOG_WARNING,
                           "Logger process: got unexpected signal (number: %d).", si.ssi_signo);
                    break;
            }
        }
        /* DEBUG */
        syslog(LOG_INFO, "Logger process: in loop");
        sleep(10);
        /*********/
    }

    // Clean up
    fclose(log_fp);                 // Log file descriptor
    free(log_params);               // Log params struct
    pthread_mutex_destroy(&lq_mtx); // Mutex for log queue
    pthread_mutexattr_destroy(&attrmutex); // Mutex attr

    // Close the signal file descriptor
    close(sfd);
    // Remove the sighal handlers
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // Write an exit code to the system log
    syslog(LOG_INFO, "Logger process stopped with status code %d.", exit_code);
    // Close the system log
    closelog();

    return exit_code;
}


void handle_log_queue(GQueue* log_queue, FILE* log_fp) {
    // When log_queue is not empty, write its elements to log_fp,
    // and pop them from log_queue
    if (!g_queue_is_empty(log_queue)) {
        /* Debug */
        syslog(LOG_INFO, "length is %d", g_queue_get_length(log_queue));
        /*
         * Use lock here
         */
        while(!g_queue_is_empty(log_queue)) {
            // Write log records until the log queue is not empty
            if (pthread_mutex_lock(&lq_mtx) == SUCCEED) {
                fprintf(log_fp, get_log_rec(g_queue_peek_head_link(log_queue)));
                g_queue_pop_head(log_queue);
                pthread_mutex_unlock(&lq_mtx);
            }
        }
        /*******/
        syslog(LOG_INFO, "length is %d", g_queue_get_length(log_queue));

        // Flush data to disk
        fflush(log_fp);
    }
}


void to_log_queue(GQueue* log_queue, char* rec) {
    // TODO: should be changed to more efficient way
    // to prevent waiting
    log_record* l_rec = make_lrec(rec);

    if (pthread_mutex_lock(&lq_mtx) == SUCCEED) {
        g_queue_push_tail(log_queue, l_rec);
        pthread_mutex_unlock(&lq_mtx);
    }
}


// Initialize mutex for log queue
static void init_log_queue_mutex(void) {
    unsigned rc = EXIT_SUCCESS;

    rc = pthread_mutexattr_init(&attrmutex);
    if (rc != SUCCEED) {
        syslog(LOG_ERR, "Logger process: pthread_mutexattr_init() cannot "
                        "initialize mutexattr, errc %d", rc);
        exit(rc);
    }

    rc = pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
    if (rc != SUCCEED) {
        syslog(LOG_ERR, "Logger process: pthread_mutexattr_setpshared() cannot "
                        "set mutexattr shared, errc %d", rc);
        exit(rc);
    }

    rc = pthread_mutex_init(&lq_mtx, &attrmutex);
    if (rc != SUCCEED) {
        syslog(LOG_ERR, "Logger process: pthread_mutex_init() cannot "
                        "initialize mutex, errc %d", rc);
        exit(rc);
    }
}


// Open log file in "a+" mode
static FILE* open_log(char* log_fpath) {
    FILE* log_fp = fopen(log_fpath, "a+");
    if (!log_fp) {
        syslog(LOG_ERR, "could not open the log file %s", log_fpath);
        exit(1);
    }

    /* DEBUG */
    fprintf(log_fp, "Logger 'hello'\n");
    fflush(log_fp);
    /**/
    return log_fp;
}
