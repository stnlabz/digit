/*
 * STN-LABZ
 * Digit Core
 *
 * digit_modules.c
 *
 * Thin Digit host integration for STN-LABZ Module ABI 1.4.
 *
 * Digit owns:
 *
 * - executable-relative module location;
 * - Digit artifact naming;
 * - operator-visible lifecycle reporting;
 * - persistent Digit audit reporting;
 * - host-service callbacks.
 *
 * STN-LABZ ABI owns:
 *
 * - module descriptors;
 * - loader state;
 * - registry state;
 * - verification;
 * - qualification;
 * - authorization;
 * - activation;
 * - STOPPED;
 * - UNREGISTERED;
 * - controlled unload boundary.
 *
 * Digit does not maintain a duplicate module lifecycle
 * implementation.
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "abi.h"
#include "digit_audit.h"
#include "digit_modules.h"
#include "module.h"
#include "module_loader.h"
#include "module_registry.h"


#define DIGIT_MODULE_PATH_MAX \
    1024

#define DIGIT_MODULE_CONF_NAME \
    "module.conf"

#define DIGIT_MODULE_AUDIT_MESSAGE_MAX \
    512


/*
 * ------------------------------------------------
 * ABI STATE
 * ------------------------------------------------
 */

static stnlabz_module_registry_t
    g_module_registry;

static stnlabz_module_loader_t
    g_module_loader;

static char g_modules_path[
    DIGIT_MODULE_PATH_MAX
];

static int g_modules_initialized =
    0;


/*
 * ------------------------------------------------
 * HOST SERVICES
 * ------------------------------------------------
 *
 * Current Digit modules do not require command-style
 * host callbacks.
 */

static const stnlabz_module_host_t
    g_module_host =
{
    NULL,
    NULL,
    NULL
};


/*
 * ------------------------------------------------
 * AUDIT
 * ------------------------------------------------
 */

static void digit_modules_audit(
    const char *message
)
{
    if (
        message == NULL ||
        message[0] == '\0'
    )
    {
        return;
    }


    (void)digit_audit_append(
        "MODULE",
        message
    );
}


/*
 * ------------------------------------------------
 * ENSURE DIRECTORY
 * ------------------------------------------------
 */

static int digit_modules_ensure_directory(
    const char *path
)
{
    DWORD attributes;


    if (
        path == NULL ||
        path[0] == '\0'
    )
    {
        return 0;
    }


    attributes =
        GetFileAttributesA(
            path
        );


    if (
        attributes !=
        INVALID_FILE_ATTRIBUTES
    )
    {
        return
            (
                attributes &
                FILE_ATTRIBUTE_DIRECTORY
            )
            ? 1
            : 0;
    }


    if (
        CreateDirectoryA(
            path,
            NULL
        )
    )
    {
        return 1;
    }


    return
        GetLastError() ==
        ERROR_ALREADY_EXISTS;
}


/*
 * ------------------------------------------------
 * PATH JOIN
 * ------------------------------------------------
 */

static int digit_modules_join_path(
    char *output,
    size_t output_size,
    const char *left,
    const char *right
)
{
    int written;


    if (
        output == NULL ||
        output_size == 0U ||
        left == NULL ||
        right == NULL
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

static int digit_modules_resolve_path(
    char *modules_path,
    size_t modules_path_size
)
{
    char executable_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char *separator;

    DWORD length;

    int written;


    if (
        modules_path == NULL ||
        modules_path_size == 0U
    )
    {
        return 0;
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
        length == 0U ||
        length >=
            sizeof(executable_path)
    )
    {
        return 0;
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
        return 0;
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
        return 0;
    }


    return 1;
}


/*
 * ------------------------------------------------
 * MODULE.CONF
 * ------------------------------------------------
 *
 * Accepted:
 *
 *     id=<module-id>
 *
 * Blank lines and '#' comments are permitted.
 *
 * Unknown keys, duplicate id declarations, malformed
 * lines, and empty IDs are rejected.
 */

static int digit_modules_read_id(
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
        module_id_size == 0U
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
            length > 0U &&
            (
                line[length - 1U] == '\n' ||
                line[length - 1U] == '\r'
            )
        )
        {
            line[
                length - 1U
            ] =
                '\0';

            length--;
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
            length == 0U ||
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
            length + 1U
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
 * INITIALIZE ABI
 * ------------------------------------------------
 */

static int digit_modules_initialize_abi(void)
{
    char message[
        DIGIT_MODULE_AUDIT_MESSAGE_MAX
    ];

    int written;


    stnlabz_module_registry_init(
        &g_module_registry
    );


    stnlabz_module_loader_init(
        &g_module_loader
    );


    memset(
        g_modules_path,
        0,
        sizeof(g_modules_path)
    );


    if (
        !digit_modules_resolve_path(
            g_modules_path,
            sizeof(g_modules_path)
        )
    )
    {
        fputs(
            "[CORE] Module path resolution FAILED\n",
            stderr
        );


        digit_modules_audit(
            "Module path resolution failed."
        );


        return 0;
    }


    if (
        !digit_modules_ensure_directory(
            g_modules_path
        )
    )
    {
        fprintf(
            stderr,
            "[CORE] Module directory FAILED: %s\n",
            g_modules_path
        );


        digit_modules_audit(
            "Module directory initialization failed."
        );


        return 0;
    }


    printf(
        "[CORE] Module path: %s\n",
        g_modules_path
    );


    written =
        snprintf(
            message,
            sizeof(message),
            "Module directory: path=%s",
            g_modules_path
        );


    if (
        written >= 0 &&
        (size_t)written <
            sizeof(message)
    )
    {
        digit_modules_audit(
            message
        );
    }


    return 1;
}


/*
 * ------------------------------------------------
 * DISCOVERY
 * ------------------------------------------------
 *
 * Digit retains only its host-specific deployment
 * convention:
 *
 *     modules\<module-id>\
 *         module.conf
 *         digit_<module-id>.dll
 *
 * Loader and descriptor validation are ABI-owned.
 */

static int digit_modules_discover(void)
{
    char search_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char directory_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char config_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char dll_name[
        STNLABZ_MODULE_ID_MAX + 16U
    ];

    char dll_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char module_id[
        STNLABZ_MODULE_ID_MAX
    ];

    WIN32_FIND_DATAA find_data;

    HANDLE search;

    size_t directories_examined =
        0U;

    size_t modules_loaded =
        0U;

    size_t modules_discovered =
        0U;

    size_t modules_rejected =
        0U;

    int written;


    written =
        snprintf(
            search_path,
            sizeof(search_path),
            "%s\\*",
            g_modules_path
        );


    if (
        written < 0 ||
        (size_t)written >=
            sizeof(search_path)
    )
    {
        return 0;
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
        /*
         * Empty module root is valid.
         */

        printf(
            "[MODULE] Discovery: 0 directories, 0 loaded, 0 discovered, 0 rejected\n"
        );


        return 1;
    }


    do
    {
        const stnlabz_module_descriptor_t
            *descriptor =
                NULL;

        stnlabz_module_loader_result_t
            loader_result;

        stnlabz_module_result_t
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


        directories_examined++;


        printf(
            "[MODULE] Examining directory: %s\n",
            find_data.cFileName
        );


        if (
            !digit_modules_join_path(
                directory_path,
                sizeof(directory_path),
                g_modules_path,
                find_data.cFileName
            )
        )
        {
            modules_rejected++;

            printf(
                "[MODULE] REJECTED: %s (DIRECTORY_PATH_INVALID)\n",
                find_data.cFileName
            );

            continue;
        }


        if (
            !digit_modules_join_path(
                config_path,
                sizeof(config_path),
                directory_path,
                DIGIT_MODULE_CONF_NAME
            )
        )
        {
            modules_rejected++;

            printf(
                "[MODULE] REJECTED: %s (MODULE_CONF_PATH_INVALID)\n",
                find_data.cFileName
            );

            continue;
        }


        if (
            !digit_modules_read_id(
                config_path,
                module_id,
                sizeof(module_id)
            )
        )
        {
            modules_rejected++;

            printf(
                "[MODULE] REJECTED: %s (MODULE_CONF_INVALID_OR_MISSING)\n",
                find_data.cFileName
            );


            printf(
                "[MODULE] Expected config: %s\n",
                config_path
            );

            continue;
        }


        printf(
            "[MODULE] Declared ID: %s\n",
            module_id
        );


        /*
         * Digit's artifact naming remains host-owned.
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
            modules_rejected++;

            printf(
                "[MODULE] REJECTED: %s (DLL_NAME_INVALID)\n",
                module_id
            );

            continue;
        }


        if (
            !digit_modules_join_path(
                dll_path,
                sizeof(dll_path),
                directory_path,
                dll_name
            )
        )
        {
            modules_rejected++;

            printf(
                "[MODULE] REJECTED: %s (DLL_PATH_INVALID)\n",
                module_id
            );

            continue;
        }


        printf(
            "[MODULE] DLL path: %s\n",
            dll_path
        );


        loader_result =
            stnlabz_module_loader_load(
                &g_module_loader,
                module_id,
                dll_path,
                &descriptor
            );


        if (
            loader_result !=
            STNLABZ_MODULE_LOADER_OK
        )
        {
            modules_rejected++;


            printf(
                "[MODULE] REJECTED: %s (LOADER_%s)\n",
                module_id,
                stnlabz_module_loader_result_string(
                    loader_result
                )
            );


            continue;
        }


        modules_loaded++;


        printf(
            "[MODULE] DLL loaded: %s\n",
            module_id
        );


        registry_result =
            stnlabz_module_registry_discover(
                &g_module_registry,
                descriptor
            );


        if (
            registry_result !=
            STNLABZ_MODULE_OK
        )
        {
            modules_rejected++;


            printf(
                "[MODULE] REJECTED: %s (REGISTRY_%s)\n",
                module_id,
                stnlabz_module_result_string(
                    registry_result
                )
            );


            (void)
            stnlabz_module_loader_unload(
                &g_module_loader,
                module_id
            );


            continue;
        }


        modules_discovered++;


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


    printf(
        "[MODULE] Discovery: "
        "%u directories, "
        "%u loaded, "
        "%u discovered, "
        "%u rejected\n",

        (unsigned int)
        directories_examined,

        (unsigned int)
        modules_loaded,

        (unsigned int)
        modules_discovered,

        (unsigned int)
        modules_rejected
    );


    return 1;
}


/*
 * ------------------------------------------------
 * PROCESS MODULES
 * ------------------------------------------------
 *
 * Qualification is intentionally live on startup.
 *
 * Digit no longer carries a second persistent
 * qualification-inventory implementation.
 *
 * Every loaded revision demonstrates qualification
 * before activation.
 */

static int digit_modules_process(void)
{
    size_t index;


    for (
        index = 0U;
        index < g_module_registry.count;
        ++index
    )
    {
        stnlabz_module_record_t
            *record;

        stnlabz_module_result_t
            result;


        record =
            &g_module_registry.modules[index];


        printf(
            "[MODULE] Discovered: %s (%s)\n",
            record->descriptor.name,
            record->descriptor.id
        );


        result =
            stnlabz_module_registry_verify(
                &g_module_registry,
                record->descriptor.id
            );


        if (
            result !=
            STNLABZ_MODULE_OK
        )
        {
            printf(
                "[MODULE] Verification FAILED: %s (%s)\n",
                record->descriptor.id,
                stnlabz_module_result_string(
                    result
                )
            );


            return 0;
        }


        printf(
            "[MODULE] Verification PASS: %s\n",
            record->descriptor.id
        );


        printf(
            "[MODULE] Qualification starting: %s\n",
            record->descriptor.id
        );


        result =
            stnlabz_module_registry_qualify(
                &g_module_registry,
                record->descriptor.id
            );


        if (
            result !=
            STNLABZ_MODULE_OK
        )
        {
            printf(
                "[MODULE] Qualification FAILED: %s (%s)\n",
                record->descriptor.id,
                stnlabz_module_result_string(
                    result
                )
            );


            return 0;
        }


        printf(
            "[MODULE] Qualification PASS: %s (%u/%u)\n",
            record->descriptor.id,
            record->qualification.tests_passed,
            record->qualification.tests_executed
        );


        printf(
            "[MODULE] Negative validation: %s\n",
            (
                record
                    ->qualification
                    .negative_test_executed &&
                record
                    ->qualification
                    .negative_test_passed
            )
            ? "PASS"
            : "FAILED"
        );


        printf(
            "[MODULE] Activation starting: %s\n",
            record->descriptor.id
        );


        result =
            stnlabz_module_abi_authorize_and_activate(
                &g_module_registry,
                record->descriptor.id,
                &g_module_host
            );


        if (
            result !=
            STNLABZ_MODULE_OK
        )
        {
            printf(
                "[MODULE] Activation FAILED: %s (%s)\n",
                record->descriptor.id,
                stnlabz_module_result_string(
                    result
                )
            );


            return 0;
        }


        printf(
            "[MODULE] ACTIVE: %s\n",
            record->descriptor.id
        );
    }


    return 1;
}


/*
 * ------------------------------------------------
 * PUBLIC INITIALIZATION
 * ------------------------------------------------
 */

int digit_modules_init(void)
{
    if (
        g_modules_initialized
    )
    {
        return -1;
    }


    if (
        !digit_modules_initialize_abi()
    )
    {
        return -1;
    }


    if (
        !digit_modules_discover()
    )
    {
        return -1;
    }


    if (
        !digit_modules_process()
    )
    {
        digit_modules_shutdown();

        return -1;
    }


    g_modules_initialized =
        1;


    digit_modules_audit(
        "Core module subsystem initialized through STN-LABZ ABI 1.4."
    );


    return 0;
}


/*
 * ------------------------------------------------
 * READY
 * ------------------------------------------------
 */

int digit_modules_is_ready(void)
{
    return
        g_modules_initialized;
}


/*
 * ------------------------------------------------
 * MODULE EXPORT ACCESS
 * ------------------------------------------------
 *
 * Capability-specific exports remain module-owned.
 *
 * Core first proves the module is ACTIVE through the
 * ABI registry, then resolves the requested symbol
 * from the ABI-owned loader handle.
 */

FARPROC digit_modules_get_export(
    const char *module_id,
    const char *export_name
)
{
    const stnlabz_module_record_t
        *record;

    const stnlabz_loaded_module_t
        *loaded;


    if (
        !g_modules_initialized ||
        module_id == NULL ||
        export_name == NULL ||
        module_id[0] == '\0' ||
        export_name[0] == '\0'
    )
    {
        return NULL;
    }


    record =
        stnlabz_module_registry_find(
            &g_module_registry,
            module_id
        );


    if (
        record == NULL ||
        record->state !=
            STNLABZ_MODULE_STATE_ACTIVE
    )
    {
        return NULL;
    }


    loaded =
        stnlabz_module_loader_find(
            &g_module_loader,
            module_id
        );


    if (
        loaded == NULL ||
        loaded->handle == NULL
    )
    {
        return NULL;
    }


    return
        GetProcAddress(
            loaded->handle,
            export_name
        );
}


/*
 * ------------------------------------------------
 * SHUTDOWN
 * ------------------------------------------------
 *
 * ABI 1.4 controlled shutdown:
 *
 *     ACTIVE
 *       -> STOPPED
 *       -> UNREGISTERED
 *       -> DLL unload
 *
 * Core never unloads a DLL while the ABI registry
 * still represents that module as ACTIVE.
 */

void digit_modules_shutdown(void)
{
    while (
        g_module_loader.count > 0U
    )
    {
        const stnlabz_loaded_module_t
            *loaded;

        const stnlabz_module_record_t
            *record;

        char module_id[
            STNLABZ_MODULE_ID_MAX
        ];

        stnlabz_module_result_t
            result;

        stnlabz_module_loader_result_t
            loader_result;


        loaded =
            &g_module_loader.modules[
                g_module_loader.count - 1U
            ];


        if (
            strcpy_s(
                module_id,
                sizeof(module_id),
                loaded->module_id
            ) != 0
        )
        {
            break;
        }


        record =
            stnlabz_module_registry_find(
                &g_module_registry,
                module_id
            );


        if (
            record != NULL &&
            record->state ==
                STNLABZ_MODULE_STATE_ACTIVE
        )
        {
            result =
                stnlabz_module_abi_stop(
                    &g_module_registry,
                    module_id
                );


            if (
                result !=
                STNLABZ_MODULE_OK
            )
            {
                digit_modules_audit(
                    "Module stop failed during shutdown."
                );

                break;
            }
        }


        record =
            stnlabz_module_registry_find(
                &g_module_registry,
                module_id
            );


        if (
            record != NULL
        )
        {
            result =
                stnlabz_module_abi_unregister(
                    &g_module_registry,
                    module_id
                );


            if (
                result !=
                STNLABZ_MODULE_OK
            )
            {
                digit_modules_audit(
                    "Module unregister failed during shutdown."
                );

                break;
            }
        }


        loader_result =
            stnlabz_module_loader_unload(
                &g_module_loader,
                module_id
            );


        if (
            loader_result !=
                STNLABZ_MODULE_LOADER_OK
        )
        {
            digit_modules_audit(
                "Module DLL unload failed during shutdown."
            );

            break;
        }
    }


    g_modules_initialized =
        0;
}