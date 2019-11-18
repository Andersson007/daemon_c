#ifndef LOGGER_H
#define LOGGER_H

#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/signalfd.h>
#include <unistd.h>

typedef struct log_record {
    char* rec;
} log_record;

log_record* make_lrec(char* rec);

int logger(void* udata);

#endif
