#ifndef DIGIT_MODULE_INVENTORY_H
#define DIGIT_MODULE_INVENTORY_H

#include <stddef.h>

#include "digit_module.h"


#define DIGIT_MODULE_INVENTORY_MAX \
    32

#define DIGIT_MODULE_INVENTORY_PATH_MAX \
    1024


typedef enum
{
    DIGIT_MODULE_INVENTORY_OK = 0,

    DIGIT_MODULE_INVENTORY_ERR_INVALID_ARGUMENT,

    DIGIT_MODULE_INVENTORY_ERR_PATH_TOO_LONG,

    DIGIT_MODULE_INVENTORY_ERR_OPEN_FAILED,

    DIGIT_MODULE_INVENTORY_ERR_READ_FAILED,

    DIGIT_MODULE_INVENTORY_ERR_WRITE_FAILED,

    DIGIT_MODULE_INVENTORY_ERR_INVALID_FORMAT,

    DIGIT_MODULE_INVENTORY_ERR_FULL

} digit_module_inventory_result_t;


/*
 * ------------------------------------------------
 * PERSISTED QUALIFICATION RECORD
 * ------------------------------------------------
 */

typedef struct
{
    char module_id[
        DIGIT_MODULE_ID_MAX
    ];

    unsigned int version_major;

    unsigned int version_minor;

    unsigned int version_patch;


    unsigned int core_api_major;

    unsigned int core_api_minor;


    digit_module_qualification_result_t qualification;

} digit_module_inventory_record_t;


/*
 * ------------------------------------------------
 * INVENTORY
 * ------------------------------------------------
 */

typedef struct
{
    digit_module_inventory_record_t
        records[
            DIGIT_MODULE_INVENTORY_MAX
        ];

    size_t count;


    char path[
        DIGIT_MODULE_INVENTORY_PATH_MAX
    ];

} digit_module_inventory_t;


/*
 * ------------------------------------------------
 * API
 * ------------------------------------------------
 */

void digit_module_inventory_init(
    digit_module_inventory_t* inventory
);


digit_module_inventory_result_t
digit_module_inventory_configure(
    digit_module_inventory_t* inventory,
    const char* state_path
);


digit_module_inventory_result_t
digit_module_inventory_load(
    digit_module_inventory_t* inventory
);


digit_module_inventory_result_t
digit_module_inventory_store(
    digit_module_inventory_t* inventory,
    const digit_module_descriptor_t* descriptor,
    const digit_module_qualification_result_t* qualification
);


const digit_module_inventory_record_t*
digit_module_inventory_find(
    const digit_module_inventory_t* inventory,
    const digit_module_descriptor_t* descriptor
);


const char*
digit_module_inventory_result_string(
    digit_module_inventory_result_t result
);


#endif