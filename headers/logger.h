#ifndef LOGGER_H
#define LOGGER_H

#include <glib.h>

typedef struct log_record {
    char* rec;
} log_record;

log_record* make_lrec(char* rec);

#endif
