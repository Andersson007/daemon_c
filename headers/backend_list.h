#ifndef PROC_LIST_H
#define PROC_LIST_H

#define CONTROL_PROCESS 1

typedef struct backend_node {
        pid_t b_pid;
        unsigned b_type;
        struct backend_node *next;
} backend_node;

backend_node* init_backend_list(void);

//void register_backend(backend_node *backend_list, pid_t backend_pid, unsigned type);

#endif
