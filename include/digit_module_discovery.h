#ifndef DIGIT_MODULE_DISCOVERY_H
#define DIGIT_MODULE_DISCOVERY_H

#include <stddef.h>

#include "digit_module.h"
#include "digit_module_loader.h"
#include "digit_module_registry.h"


typedef struct
{
    size_t directories_examined;

    size_t modules_loaded;

    size_t modules_discovered;

    size_t modules_rejected;

} digit_module_discovery_report_t;


/*
 * Resolve the module directory relative to
 * digit.exe:
 *
 *     <digit.exe directory>\modules
 */

digit_module_result_t
digit_module_discovery_get_path(
    char* modules_path,
    size_t modules_path_size
);


/*
 * Scan the module directory, load DLL modules,
 * obtain their descriptors through the established
 * ABI, and submit valid descriptors to the Core
 * registry.
 */

digit_module_result_t
digit_module_discovery_scan(
    digit_module_registry_t* registry,
    digit_module_loader_t* loader,
    const char* modules_path,
    digit_module_discovery_report_t* report
);


#endif