#ifndef PROC_LIST_H
#define PROC_LIST_H

#define CONTROL_PROCESS 1
#define LOGGER_PROCESS 2

#include <glib.h>

typedef struct backend_node {
    pid_t b_pid;
    unsigned b_type;
} backend_node;

backend_node* make_backend(pid_t b_pid, unsigned b_type);

#endif
