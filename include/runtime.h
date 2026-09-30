#ifndef DIGIT_RUNTIME_H
#define DIGIT_RUNTIME_H

#include "hotload.h"
#include "module_manager.h"

typedef struct
{
    digit_module_manager_t *modules;
    digit_hotload_t hotload;
    int running;
} digit_runtime_t;

void digit_runtime_init(
    digit_runtime_t *runtime,
    digit_module_manager_t *modules
);

int digit_runtime_run(
    digit_runtime_t *runtime
);

void digit_runtime_request_stop(void);

#endif
