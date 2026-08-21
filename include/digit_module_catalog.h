#ifndef DIGIT_MODULE_CATALOG_H
#define DIGIT_MODULE_CATALOG_H

#include <stddef.h>

#include "digit_module.h"


#define DIGIT_MODULE_CATALOG_MAX \
    32


typedef struct
{
    const digit_module_descriptor_t*
        entries[
            DIGIT_MODULE_CATALOG_MAX
        ];

    size_t count;

} digit_module_catalog_t;


void digit_module_catalog_init(
    digit_module_catalog_t* catalog
);


digit_module_result_t
digit_module_catalog_register(
    digit_module_catalog_t* catalog,
    const digit_module_descriptor_t* descriptor
);


const digit_module_descriptor_t*
digit_module_catalog_find(
    const digit_module_catalog_t* catalog,
    const char* module_id
);


#endif