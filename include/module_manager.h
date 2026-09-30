#ifndef DIGIT_MODULE_MANAGER_H
#define DIGIT_MODULE_MANAGER_H

#include <stddef.h>

#include "module.h"
#include "module_discovery.h"
#include "module_loader.h"
#include "module_registry.h"
#include "qualification.h"

#define DIGIT_QUALIFICATION_STATE_FILE "qualification.state"

typedef struct
{
    stnlabz_module_registry_t registry;
    stnlabz_module_loader_t loader;
    stnlabz_module_host_t host;
    digit_qualification_inventory_t qualifications;
    char modules_path[STNLABZ_MODULE_LOADER_PATH_MAX];
    char qualification_path[STNLABZ_MODULE_LOADER_PATH_MAX];
} digit_module_manager_t;

void digit_module_manager_init(
    digit_module_manager_t *manager,
    const stnlabz_module_host_t *host
);

stnlabz_module_result_t digit_module_manager_discover(
    digit_module_manager_t *manager,
    stnlabz_module_discovery_report_t *report
);

stnlabz_module_result_t digit_module_manager_qualify_and_activate(
    digit_module_manager_t *manager
);

void digit_module_manager_shutdown(
    digit_module_manager_t *manager
);

size_t digit_module_manager_count(
    const digit_module_manager_t *manager
);

const char *digit_module_manager_path(
    const digit_module_manager_t *manager
);

#endif
