#ifndef CONTROL_PROCESS_H
#define CONTROL_PROCESS_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/signalfd.h>
#include <unistd.h>

typedef struct cproc_params {
    int argc;
    char** argv;
} cproc_params;

int control_process(void *udata);

#endif
