#define _GNU_SOURCE
#include <stddef.h>
#include <time.h>
#include "headers/timec.h"

void msleep(unsigned long msec) {
    struct timespec ts;
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
