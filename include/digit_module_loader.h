#ifndef DIGIT_MODULE_LOADER_H
#define DIGIT_MODULE_LOADER_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stddef.h>

#include "digit_module.h"


#define DIGIT_MODULE_LOADER_MAX \
    32

#define DIGIT_MODULE_LOADER_PATH_MAX \
    1024

#define DIGIT_MODULE_DESCRIPTOR_EXPORT \
    "digit_module_get_descriptor"


typedef enum
{
    DIGIT_MODULE_LOADER_OK = 0,

    DIGIT_MODULE_LOADER_ERR_INVALID_ARGUMENT,

    DIGIT_MODULE_LOADER_ERR_FULL,

    DIGIT_MODULE_LOADER_ERR_ALREADY_LOADED,

    DIGIT_MODULE_LOADER_ERR_LOAD_FAILED,

    DIGIT_MODULE_LOADER_ERR_EXPORT_MISSING,

    DIGIT_MODULE_LOADER_ERR_DESCRIPTOR_INVALID,

    DIGIT_MODULE_LOADER_ERR_ID_MISMATCH,

    DIGIT_MODULE_LOADER_ERR_NOT_FOUND

} digit_module_loader_result_t;


typedef const digit_module_descriptor_t*
(*digit_module_get_descriptor_fn)(void);


typedef struct
{
    HMODULE handle;

    char module_id[
        DIGIT_MODULE_ID_MAX
    ];

    char dll_path[
        DIGIT_MODULE_LOADER_PATH_MAX
    ];

    const digit_module_descriptor_t* descriptor;

} digit_loaded_module_t;


typedef struct
{
    digit_loaded_module_t modules[
        DIGIT_MODULE_LOADER_MAX
    ];

    size_t count;

} digit_module_loader_t;


void digit_module_loader_init(
    digit_module_loader_t* loader
);


digit_module_loader_result_t
digit_module_loader_load(
    digit_module_loader_t* loader,
    const char* expected_module_id,
    const char* dll_path,
    const digit_module_descriptor_t** descriptor_out
);


digit_module_loader_result_t
digit_module_loader_unload(
    digit_module_loader_t* loader,
    const char* module_id
);


void digit_module_loader_unload_all(
    digit_module_loader_t* loader
);


const digit_loaded_module_t*
digit_module_loader_find(
    const digit_module_loader_t* loader,
    const char* module_id
);


const char*
digit_module_loader_result_string(
    digit_module_loader_result_t result
);


#endif