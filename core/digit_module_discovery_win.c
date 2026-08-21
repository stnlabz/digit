/*
 * STN-LABZ
 * Digit Core
 *
 * digit_module_discovery_win.c
 *
 * Windows dynamic module discovery.
 *
 * Expected layout:
 *
 *     <digit.exe directory>\
 *         modules\
 *             sqlite\
 *                 module.conf
 *                 digit_sqlite.dll
 *
 * module.conf:
 *
 *     id=sqlite
 *
 * DLL filename is derived from the module ID:
 *
 *     digit_<id>.dll
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "digit_module_discovery.h"


#define DIGIT_DISCOVERY_PATH_MAX \
    1024

#define DIGIT_MODULE_CONF_NAME \
    "module.conf"


/*
 * ------------------------------------------------
 * PATH JOIN
 * ------------------------------------------------
 */

static int digit_discovery_join_path(
    char *output,
    size_t output_size,
    const char *left,
    const char *right
)
{
    int written;


    if (
        output == NULL ||
        left == NULL ||
        right == NULL ||
        output_size == 0
    )
    {
        return 0;
    }


    written =
        snprintf(
            output,
            output_size,
            "%s\\%s",
            left,
            right
        );


    if (
        written < 0 ||
        (size_t)written >=
            output_size
    )
    {
        return 0;
    }


    return 1;
}


/*
 * ------------------------------------------------
 * MODULE ROOT
 * ------------------------------------------------
 */

digit_module_result_t
digit_module_discovery_get_path(
    char *modules_path,
    size_t modules_path_size
)
{
    char executable_path[
        DIGIT_DISCOVERY_PATH_MAX
    ];

    char *separator;

    DWORD length;

    int written;


    if (
        modules_path == NULL ||
        modules_path_size == 0
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    memset(
        executable_path,
        0,
        sizeof(executable_path)
    );


    length =
        GetModuleFileNameA(
            NULL,
            executable_path,
            (DWORD)sizeof(executable_path)
        );


    if (
        length == 0 ||
        length >=
            sizeof(executable_path)
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    separator =
        strrchr(
            executable_path,
            '\\'
        );


    if (
        separator == NULL
    )
    {
        separator =
            strrchr(
                executable_path,
                '/'
            );
    }


    if (
        separator == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    *separator =
        '\0';


    written =
        snprintf(
            modules_path,
            modules_path_size,
            "%s\\modules",
            executable_path
        );


    if (
        written < 0 ||
        (size_t)written >=
            modules_path_size
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    return
        DIGIT_MODULE_OK;
}


/*
 * ------------------------------------------------
 * READ MODULE ID
 * ------------------------------------------------
 */

static int digit_discovery_read_module_id(
    const char *config_path,
    char *module_id,
    size_t module_id_size
)
{
    FILE *file =
        NULL;

    char line[
        512
    ];

    int id_seen =
        0;


    if (
        config_path == NULL ||
        module_id == NULL ||
        module_id_size == 0
    )
    {
        return 0;
    }


    module_id[0] =
        '\0';


    if (
        fopen_s(
            &file,
            config_path,
            "r"
        ) != 0 ||
        file == NULL
    )
    {
        return 0;
    }


    while (
        fgets(
            line,
            sizeof(line),
            file
        ) != NULL
    )
    {
        char *separator;

        char *key;

        char *value;

        size_t length;


        length =
            strlen(
                line
            );


        while (
            length > 0 &&
            (
                line[length - 1] == '\n' ||
                line[length - 1] == '\r'
            )
        )
        {
            line[
                length - 1
            ] =
                '\0';

            --length;
        }


        if (
            line[0] == '\0' ||
            line[0] == '#'
        )
        {
            continue;
        }


        separator =
            strchr(
                line,
                '='
            );


        if (
            separator == NULL
        )
        {
            fclose(
                file
            );

            return 0;
        }


        *separator =
            '\0';


        key =
            line;

        value =
            separator + 1;


        if (
            strcmp(
                key,
                "id"
            ) != 0
        )
        {
            fclose(
                file
            );

            return 0;
        }


        if (
            id_seen
        )
        {
            fclose(
                file
            );

            return 0;
        }


        length =
            strlen(
                value
            );


        if (
            length == 0 ||
            length >=
                module_id_size
        )
        {
            fclose(
                file
            );

            return 0;
        }


        memcpy(
            module_id,
            value,
            length + 1
        );


        id_seen =
            1;
    }


    fclose(
        file
    );


    return
        id_seen;
}


/*
 * ------------------------------------------------
 * DISCOVERY SCAN
 * ------------------------------------------------
 */

digit_module_result_t
digit_module_discovery_scan(
    digit_module_registry_t *registry,
    digit_module_loader_t *loader,
    const char *modules_path,
    digit_module_discovery_report_t *report
)
{
    char search_path[
        DIGIT_DISCOVERY_PATH_MAX
    ];

    char directory_path[
        DIGIT_DISCOVERY_PATH_MAX
    ];

    char config_path[
        DIGIT_DISCOVERY_PATH_MAX
    ];

    char dll_name[
        DIGIT_MODULE_ID_MAX + 16
    ];

    char dll_path[
        DIGIT_DISCOVERY_PATH_MAX
    ];

    char module_id[
        DIGIT_MODULE_ID_MAX
    ];

    WIN32_FIND_DATAA find_data;

    HANDLE search;

    int written;


    if (
        registry == NULL ||
        loader == NULL ||
        modules_path == NULL ||
        report == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    memset(
        report,
        0,
        sizeof(*report)
    );


    written =
        snprintf(
            search_path,
            sizeof(search_path),
            "%s\\*",
            modules_path
        );


    if (
        written < 0 ||
        (size_t)written >=
            sizeof(search_path)
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    search =
        FindFirstFileA(
            search_path,
            &find_data
        );


    if (
        search ==
        INVALID_HANDLE_VALUE
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    do
    {
        const digit_module_descriptor_t
            *descriptor;

        digit_module_loader_result_t
            loader_result;

        digit_module_result_t
            registry_result;


        if (
            (
                find_data.dwFileAttributes &
                FILE_ATTRIBUTE_DIRECTORY
            ) == 0
        )
        {
            continue;
        }


        if (
            strcmp(
                find_data.cFileName,
                "."
            ) == 0 ||
            strcmp(
                find_data.cFileName,
                ".."
            ) == 0
        )
        {
            continue;
        }


        report
            ->directories_examined++;


        printf(
            "[MODULE] Examining directory: %s\n",
            find_data.cFileName
        );


        /*
         * <modules>\<directory>
         */

        if (
            !digit_discovery_join_path(
                directory_path,
                sizeof(directory_path),
                modules_path,
                find_data.cFileName
            )
        )
        {
            printf(
                "[MODULE] REJECTED: %s (DIRECTORY_PATH_INVALID)\n",
                find_data.cFileName
            );

            report
                ->modules_rejected++;

            continue;
        }


        /*
         * <module-directory>\module.conf
         */

        if (
            !digit_discovery_join_path(
                config_path,
                sizeof(config_path),
                directory_path,
                DIGIT_MODULE_CONF_NAME
            )
        )
        {
            printf(
                "[MODULE] REJECTED: %s (CONFIG_PATH_INVALID)\n",
                find_data.cFileName
            );

            report
                ->modules_rejected++;

            continue;
        }


        /*
         * Read module identity.
         */

        if (
            !digit_discovery_read_module_id(
                config_path,
                module_id,
                sizeof(module_id)
            )
        )
        {
            printf(
                "[MODULE] REJECTED: %s (MODULE_CONF_INVALID_OR_MISSING)\n",
                find_data.cFileName
            );

            printf(
                "[MODULE] Expected config: %s\n",
                config_path
            );

            report
                ->modules_rejected++;

            continue;
        }


        printf(
            "[MODULE] Declared ID: %s\n",
            module_id
        );


        /*
         * Digit DLL naming:
         *
         *     digit_<id>.dll
         */

        written =
            snprintf(
                dll_name,
                sizeof(dll_name),
                "digit_%s.dll",
                module_id
            );


        if (
            written < 0 ||
            (size_t)written >=
                sizeof(dll_name)
        )
        {
            printf(
                "[MODULE] REJECTED: %s (DLL_NAME_INVALID)\n",
                module_id
            );

            report
                ->modules_rejected++;

            continue;
        }


        if (
            !digit_discovery_join_path(
                dll_path,
                sizeof(dll_path),
                directory_path,
                dll_name
            )
        )
        {
            printf(
                "[MODULE] REJECTED: %s (DLL_PATH_INVALID)\n",
                module_id
            );

            report
                ->modules_rejected++;

            continue;
        }


        printf(
            "[MODULE] DLL path: %s\n",
            dll_path
        );


        descriptor =
            NULL;


        /*
         * ------------------------------------------------
         * LOAD DLL
         * ------------------------------------------------
         */

        loader_result =
            digit_module_loader_load(
                loader,
                module_id,
                dll_path,
                &descriptor
            );


        if (
            loader_result !=
            DIGIT_MODULE_LOADER_OK
        )
        {
            printf(
                "[MODULE] REJECTED: %s (LOADER_%s)\n",
                module_id,
                digit_module_loader_result_string(
                    loader_result
                )
            );


            report
                ->modules_rejected++;

            continue;
        }


        report
            ->modules_loaded++;


        printf(
            "[MODULE] DLL loaded: %s\n",
            module_id
        );


        /*
         * ------------------------------------------------
         * REGISTER DISCOVERED MODULE
         * ------------------------------------------------
         */

        registry_result =
            digit_module_registry_discover(
                registry,
                descriptor
            );


        if (
            registry_result !=
            DIGIT_MODULE_OK
        )
        {
            printf(
                "[MODULE] REJECTED: %s (REGISTRY_%s)\n",
                module_id,
                digit_module_result_string(
                    registry_result
                )
            );


            (void)
            digit_module_loader_unload(
                loader,
                module_id
            );


            report
                ->modules_rejected++;

            continue;
        }


        report
            ->modules_discovered++;


        printf(
            "[MODULE] Discovery accepted: %s\n",
            module_id
        );

    } while (
        FindNextFileA(
            search,
            &find_data
        )
    );


    FindClose(
        search
    );


    return
        DIGIT_MODULE_OK;
}