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

typedef struct logger_params {
    GQueue* log_queue;
    char* log_fpath;
} logger_params;

typedef struct log_record {
    char* rec;
} log_record;

log_record* make_lrec(char* rec);

int logger(void* udata);

void handle_log_queue(GQueue* log_queue, FILE* log_fptr);

void to_log_queue(GQueue* log_queue, char* rec);

#endif
