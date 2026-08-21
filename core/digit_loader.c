/*
 * STN-LABZ
 * Digit Core
 *
 * digit_loader_win.c
 *
 * Windows dynamic module loader.
 *
 * Loading a DLL does not establish qualification,
 * authorization, or activation.
 *
 * Core loads only through the established ABI:
 *
 *     digit_module_get_descriptor()
 *
 * The returned descriptor is then processed by
 * Core-controlled verification and qualification.
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <string.h>

#include "digit_module_loader.h"


/*
 * ------------------------------------------------
 * DESCRIPTOR VALIDATION
 * ------------------------------------------------
 */

static int digit_module_loader_descriptor_valid(
    const digit_module_descriptor_t *descriptor
)
{
    size_t id_length;

    size_t name_length;


    if (
        descriptor == NULL
    )
    {
        return 0;
    }


    id_length =
        strnlen_s(
            descriptor->id,
            sizeof(descriptor->id)
        );


    name_length =
        strnlen_s(
            descriptor->name,
            sizeof(descriptor->name)
        );


    if (
        id_length == 0 ||
        id_length >=
            sizeof(descriptor->id)
    )
    {
        return 0;
    }


    if (
        name_length == 0 ||
        name_length >=
            sizeof(descriptor->name)
    )
    {
        return 0;
    }


    if (
        descriptor->qualify ==
        NULL
    )
    {
        return 0;
    }


    return 1;
}


/*
 * ------------------------------------------------
 * INITIALIZATION
 * ------------------------------------------------
 */

void digit_module_loader_init(
    digit_module_loader_t *loader
)
{
    if (
        loader == NULL
    )
    {
        return;
    }


    memset(
        loader,
        0,
        sizeof(*loader)
    );
}


/*
 * ------------------------------------------------
 * FIND
 * ------------------------------------------------
 */

const digit_loaded_module_t *
digit_module_loader_find(
    const digit_module_loader_t *loader,
    const char *module_id
)
{
    size_t index;


    if (
        loader == NULL ||
        module_id == NULL
    )
    {
        return NULL;
    }


    for (
        index = 0;
        index < loader->count;
        ++index
    )
    {
        if (
            strcmp(
                loader
                    ->modules[index]
                    .module_id,
                module_id
            ) == 0
        )
        {
            return
                &loader->modules[index];
        }
    }


    return NULL;
}


/*
 * ------------------------------------------------
 * LOAD
 * ------------------------------------------------
 */

digit_module_loader_result_t
digit_module_loader_load(
    digit_module_loader_t *loader,
    const char *expected_module_id,
    const char *dll_path,
    const digit_module_descriptor_t **descriptor_out
)
{
    HMODULE handle;

    FARPROC export_address;

    digit_module_get_descriptor_fn
        get_descriptor;

    const digit_module_descriptor_t
        *descriptor;

    digit_loaded_module_t
        *loaded;

    size_t id_length;

    size_t path_length;


    if (
        loader == NULL ||
        expected_module_id == NULL ||
        dll_path == NULL ||
        descriptor_out == NULL ||
        expected_module_id[0] == '\0' ||
        dll_path[0] == '\0'
    )
    {
        return
            DIGIT_MODULE_LOADER_ERR_INVALID_ARGUMENT;
    }


    *descriptor_out =
        NULL;


    if (
        digit_module_loader_find(
            loader,
            expected_module_id
        ) != NULL
    )
    {
        return
            DIGIT_MODULE_LOADER_ERR_ALREADY_LOADED;
    }


    if (
        loader->count >=
        DIGIT_MODULE_LOADER_MAX
    )
    {
        return
            DIGIT_MODULE_LOADER_ERR_FULL;
    }


    id_length =
        strlen(
            expected_module_id
        );


    path_length =
        strlen(
            dll_path
        );


    if (
        id_length == 0 ||
        id_length >=
            DIGIT_MODULE_ID_MAX ||
        path_length == 0 ||
        path_length >=
            DIGIT_MODULE_LOADER_PATH_MAX
    )
    {
        return
            DIGIT_MODULE_LOADER_ERR_INVALID_ARGUMENT;
    }


    /*
     * ------------------------------------------------
     * LOAD DLL
     * ------------------------------------------------
     *
     * Full DLL path is supplied by Core discovery.
     *
     * LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR allows the
     * module's own directory to satisfy legitimate
     * DLL dependencies.
     *
     * LOAD_LIBRARY_SEARCH_DEFAULT_DIRS avoids the
     * legacy current-working-directory search path.
     */

    handle =
        LoadLibraryExA(
            dll_path,
            NULL,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
            LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
        );


    if (
        handle == NULL
    )
    {
        return
            DIGIT_MODULE_LOADER_ERR_LOAD_FAILED;
    }


    /*
     * ------------------------------------------------
     * ABI EXPORT
     * ------------------------------------------------
     */

    export_address =
        GetProcAddress(
            handle,
            DIGIT_MODULE_DESCRIPTOR_EXPORT
        );


    if (
        export_address == NULL
    )
    {
        FreeLibrary(
            handle
        );


        return
            DIGIT_MODULE_LOADER_ERR_EXPORT_MISSING;
    }


    get_descriptor =
        (digit_module_get_descriptor_fn)
        export_address;


    /*
     * ------------------------------------------------
     * DESCRIPTOR
     * ------------------------------------------------
     */

    descriptor =
        get_descriptor();


    if (
        !digit_module_loader_descriptor_valid(
            descriptor
        )
    )
    {
        FreeLibrary(
            handle
        );


        return
            DIGIT_MODULE_LOADER_ERR_DESCRIPTOR_INVALID;
    }


    /*
     * Filesystem declaration and DLL identity must
     * agree exactly.
     */

    if (
        strcmp(
            descriptor->id,
            expected_module_id
        ) != 0
    )
    {
        FreeLibrary(
            handle
        );


        return
            DIGIT_MODULE_LOADER_ERR_ID_MISMATCH;
    }


    /*
     * ------------------------------------------------
     * RETAIN MODULE
     * ------------------------------------------------
     *
     * The DLL must remain loaded while Core holds
     * descriptor function pointers from it.
     */

    loaded =
        &loader->modules[
            loader->count
        ];


    memset(
        loaded,
        0,
        sizeof(*loaded)
    );


    loaded->handle =
        handle;


    memcpy(
        loaded->module_id,
        expected_module_id,
        id_length + 1
    );


    memcpy(
        loaded->dll_path,
        dll_path,
        path_length + 1
    );


    loaded->descriptor =
        descriptor;


    loader->count++;


    *descriptor_out =
        descriptor;


    return
        DIGIT_MODULE_LOADER_OK;
}


/*
 * ------------------------------------------------
 * UNLOAD
 * ------------------------------------------------
 */

digit_module_loader_result_t
digit_module_loader_unload(
    digit_module_loader_t *loader,
    const char *module_id
)
{
    size_t index;


    if (
        loader == NULL ||
        module_id == NULL ||
        module_id[0] == '\0'
    )
    {
        return
            DIGIT_MODULE_LOADER_ERR_INVALID_ARGUMENT;
    }


    for (
        index = 0;
        index < loader->count;
        ++index
    )
    {
        if (
            strcmp(
                loader
                    ->modules[index]
                    .module_id,
                module_id
            ) == 0
        )
        {
            size_t move_index;


            if (
                loader
                    ->modules[index]
                    .handle != NULL
            )
            {
                FreeLibrary(
                    loader
                        ->modules[index]
                        .handle
                );
            }


            for (
                move_index = index;
                move_index + 1 <
                    loader->count;
                ++move_index
            )
            {
                loader->modules[
                    move_index
                ] =
                    loader->modules[
                        move_index + 1
                    ];
            }


            memset(
                &loader->modules[
                    loader->count - 1
                ],
                0,
                sizeof(
                    loader->modules[
                        loader->count - 1
                    ]
                )
            );


            loader->count--;


            return
                DIGIT_MODULE_LOADER_OK;
        }
    }


    return
        DIGIT_MODULE_LOADER_ERR_NOT_FOUND;
}


/*
 * ------------------------------------------------
 * UNLOAD ALL
 * ------------------------------------------------
 */

void digit_module_loader_unload_all(
    digit_module_loader_t *loader
)
{
    if (
        loader == NULL
    )
    {
        return;
    }


    while (
        loader->count > 0
    )
    {
        size_t index;


        index =
            loader->count - 1;


        if (
            loader
                ->modules[index]
                .handle != NULL
        )
        {
            FreeLibrary(
                loader
                    ->modules[index]
                    .handle
            );
        }


        memset(
            &loader->modules[index],
            0,
            sizeof(
                loader->modules[index]
            )
        );


        loader->count--;
    }
}


/*
 * ------------------------------------------------
 * RESULT STRING
 * ------------------------------------------------
 */

const char *
digit_module_loader_result_string(
    digit_module_loader_result_t result
)
{
    switch (
        result
    )
    {
        case DIGIT_MODULE_LOADER_OK:

            return "OK";


        case DIGIT_MODULE_LOADER_ERR_INVALID_ARGUMENT:

            return "INVALID_ARGUMENT";


        case DIGIT_MODULE_LOADER_ERR_FULL:

            return "FULL";


        case DIGIT_MODULE_LOADER_ERR_ALREADY_LOADED:

            return "ALREADY_LOADED";


        case DIGIT_MODULE_LOADER_ERR_LOAD_FAILED:

            return "LOAD_FAILED";


        case DIGIT_MODULE_LOADER_ERR_EXPORT_MISSING:

            return "EXPORT_MISSING";


        case DIGIT_MODULE_LOADER_ERR_DESCRIPTOR_INVALID:

            return "DESCRIPTOR_INVALID";


        case DIGIT_MODULE_LOADER_ERR_ID_MISMATCH:

            return "ID_MISMATCH";


        case DIGIT_MODULE_LOADER_ERR_NOT_FOUND:

            return "NOT_FOUND";


        default:

            return "UNKNOWN";
    }
}