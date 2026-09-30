#ifndef DIGIT_HOTLOAD_H
#define DIGIT_HOTLOAD_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#include "module_manager.h"

#define DIGIT_HOTLOAD_MAX_MODULES STNLABZ_MODULE_REGISTRY_MAX
#define DIGIT_HOTLOAD_PATH_MAX STNLABZ_MODULE_LOADER_PATH_MAX

typedef struct
{
    char module_id[STNLABZ_MODULE_ID_MAX];
    char path[DIGIT_HOTLOAD_PATH_MAX];
    time_t modified_time;
    off_t size;
} digit_hotload_file_t;

typedef struct
{
    digit_module_manager_t *manager;
    digit_hotload_file_t files[DIGIT_HOTLOAD_MAX_MODULES];
    size_t count;
} digit_hotload_t;

void digit_hotload_init(
    digit_hotload_t *hotload,
    digit_module_manager_t *manager
);

int digit_hotload_snapshot(
    digit_hotload_t *hotload
);

int digit_hotload_poll(
    digit_hotload_t *hotload
);

#endif
