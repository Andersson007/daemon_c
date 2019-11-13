#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "headers/backend_list.h"


backend_node* init_backend_list(void) {
    // Initialize the first element of backend_list with
    // control_process's pid and type and return a pointer

    backend_node *ret_val = malloc(sizeof(backend_node));
    if (ret_val == NULL) {
        return NULL;
    }

    ret_val->b_pid = getpid();
    ret_val->b_type = CONTROL_PROCESS;
    ret_val->next = NULL;

    return ret_val;
}
