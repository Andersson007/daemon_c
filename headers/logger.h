#ifndef LOGGER_H
#define LOGGER_H

#include <glib.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/signalfd.h>
#include <sys/time.h>
#include <unistd.h>

// Log msg levels
#define INF 0
#define WRN 1
#define ERR 2
#define DEBUG 4
#define FATAL 5
#define PANIC 6

typedef struct logger_params {
    GQueue* log_queue;
    char* log_fpath;
} logger_params;

typedef struct log_record {
    time_t ts_epoch;
    int msg_lvl;
    char* rec;
} log_record;

log_record* make_lrec(int msg_lvl, char* rec);

int logger(void* udata);

void handle_log_queue(GQueue* log_queue, FILE* log_fptr);

void to_log_queue(GQueue* log_queue, int msg_lvl, char* rec_fmt,...);

#endif
