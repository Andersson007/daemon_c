#ifndef PROC_LIST_H
#define PROC_LIST_H

#define CONTROL_PROCESS 1

#include <glib.h>

typedef struct backend_node {
    pid_t b_pid;
    unsigned b_type;
} backend_node;

inline pid_t get_b_pid(GList* item);

inline int get_b_type(GList* item);

backend_node* make_backend(pid_t b_pid, unsigned b_type);

#endif
