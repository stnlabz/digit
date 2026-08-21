/*
 * STN-LABZ
 * Digit Core
 *
 * digit_module_catalog.c
 *
 * Static Core catalog of known module implementations.
 */

#include <string.h>

#include "digit_module_catalog.h"


void digit_module_catalog_init(
    digit_module_catalog_t *catalog
)
{
    if (
        catalog == NULL
    )
    {
        return;
    }


    memset(
        catalog,
        0,
        sizeof(*catalog)
    );
}


digit_module_result_t
digit_module_catalog_register(
    digit_module_catalog_t *catalog,
    const digit_module_descriptor_t *descriptor
)
{
    size_t index;


    if (
        catalog == NULL ||
        descriptor == NULL ||
        descriptor->id[0] == '\0' ||
        descriptor->name[0] == '\0' ||
        descriptor->qualify == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    for (
        index = 0;
        index < catalog->count;
        ++index
    )
    {
        if (
            strcmp(
                catalog
                    ->entries[index]
                    ->id,
                descriptor->id
            ) == 0
        )
        {
            return
                DIGIT_MODULE_ERR_DUPLICATE;
        }
    }


    if (
        catalog->count >=
        DIGIT_MODULE_CATALOG_MAX
    )
    {
        return
            DIGIT_MODULE_ERR_REGISTRY_FULL;
    }


    catalog->entries[
        catalog->count
    ] =
        descriptor;


    catalog->count++;


    return
        DIGIT_MODULE_OK;
}


const digit_module_descriptor_t *
digit_module_catalog_find(
    const digit_module_catalog_t *catalog,
    const char *module_id
)
{
    size_t index;


    if (
        catalog == NULL ||
        module_id == NULL
    )
    {
        return NULL;
    }


    for (
        index = 0;
        index < catalog->count;
        ++index
    )
    {
        if (
            strcmp(
                catalog
                    ->entries[index]
                    ->id,
                module_id
            ) == 0
        )
        {
            return
                catalog->entries[index];
        }
    }


    return NULL;
}