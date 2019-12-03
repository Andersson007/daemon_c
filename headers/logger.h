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

// Max length of log record in bytes
#define LOG_REC_BUF_LEN 256

#define to_log_queue(log_queue, msg_lvl, msg_fmt, ...) {                    \
    snprintf(msg, LOG_REC_BUF_LEN, msg_fmt, __VA_ARGS__);                   \
    log_record* l_rec = make_lrec(msg_lvl, msg);                            \
    if (pthread_mutex_lock(&lq_mtx) == SUCCEED) {                           \
        g_queue_push_tail(log_queue, l_rec);                                \
        pthread_mutex_unlock(&lq_mtx);}                                     \
    memset(msg, 0, LOG_REC_BUF_LEN);                                        \
}

pthread_mutex_t lq_mtx;

typedef struct logger_params {
    GQueue* log_queue;
    char* log_fpath;
} logger_params;

typedef struct log_record {
    time_t ts_epoch;
    int msg_lvl;
    char msg[LOG_REC_BUF_LEN];
} log_record;

log_record* make_lrec(int msg_lvl, char* msg);

int logger(void* udata);

void handle_log_queue(GQueue* log_queue, FILE* log_fptr);

#endif
