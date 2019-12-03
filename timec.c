#define _GNU_SOURCE
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include "headers/timec.h"

void msleep(unsigned long msec) {
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, NULL);
}


char* get_now_ts_pretty(time_t ts_epoch) {
    struct tm* t_info = localtime(&ts_epoch);
    char* t_stamp;
    // TODO: pass format string through .cfg file
    char* format = "%Y/%m/%d %H:%M:%S";

    t_stamp = (char*) malloc(TS_BUFSIZE * sizeof(char) + 1);

    strftime(t_stamp, TS_BUFSIZE, format, t_info);

    return t_stamp;
}
