#define _POSIX_C_SOURCE 1
#include "headers/backend.h"
#include "headers/general.h"
#include "headers/logger.h"


int logger(void* udata) {

    GQueue* log_queue = (GQueue*) &udata;

    int exit = 0;
    int exit_code = EXIT_SUCCESS;

    int sfd = -1;
    sigset_t mask;
    struct signalfd_siginfo si;

    // Open the system log
    openlog(PROGNAME, LOG_NDELAY, LOG_DAEMON);

    // Greeting by syslog
    syslog(LOG_INFO, "logger started. PID: %d, TYPE: %d",
           getpid(), LOGGER_PROCESS);

    // Push greeting to log queue
    g_queue_push_tail(log_queue, "Control process initialized\n");

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
    while (!exit) {
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
                    exit = 1;
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
    //

    // Close the signal file descriptor
    close(sfd);
    // Remove the sighal handlers
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // Write an exit code to the system log
    syslog(LOG_INFO, "Logger stopped with status code %d.", exit_code);
    // Close the system log
    closelog();

    return exit_code;
}
