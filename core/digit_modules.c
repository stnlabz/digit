/*
 * STN-LABZ
 * Digit Core
 *
 * digit_modules.c
 *
 * Core-controlled dynamic module orchestration.
 *
 * Responsibilities:
 *
 * - executable-relative module discovery
 * - module inventory loading
 * - module verification
 * - module qualification
 * - qualification restoration
 * - qualification persistence
 * - module activation
 * - operator-visible lifecycle reporting
 * - persistent audit reporting
 * - controlled module stop
 * - DLL unload
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "digit_audit.h"
#include "digit_module.h"
#include "digit_module_discovery.h"
#include "digit_module_inventory.h"
#include "digit_module_loader.h"
#include "digit_module_registry.h"
#include "digit_modules.h"


#define DIGIT_MODULE_PATH_MAX \
    1024

#define DIGIT_MODULE_STATE_DIRECTORY \
    "C:\\stn-labz\\digit"

#define DIGIT_MODULE_AUDIT_MESSAGE_MAX \
    512


/*
 * ------------------------------------------------
 * MODULE STATE
 * ------------------------------------------------
 */

static digit_module_registry_t
    g_module_registry;

static digit_module_loader_t
    g_module_loader;

static digit_module_inventory_t
    g_module_inventory;

static char g_modules_path[
    DIGIT_MODULE_PATH_MAX
];

static int g_modules_initialized =
    0;


/*
 * ------------------------------------------------
 * MODULE HOST API
 * ------------------------------------------------
 *
 * SQLite currently requires no Core host-service
 * callbacks.
 *
 * The host structure remains present so additional
 * Digit modules may use the established ABI later.
 */

static const digit_module_host_t
    g_module_host =
{
    NULL,
    NULL,
    NULL
};


/*
 * ------------------------------------------------
 * AUDIT MESSAGE
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
 * FORMATTED AUDIT MESSAGE
 * ------------------------------------------------
 */

static void digit_modules_audit_module(
    const char *event,
    const digit_module_record_t *record
)
{
    char message[
        DIGIT_MODULE_AUDIT_MESSAGE_MAX
    ];

    int written;


    if (
        event == NULL ||
        record == NULL
    )
    {
        return;
    }


    written =
        snprintf(
            message,
            sizeof(message),
            "%s: id=%s name=%s version=%u.%u.%u state=%s",
            event,
            record->descriptor.id,
            record->descriptor.name,
            record->descriptor.version_major,
            record->descriptor.version_minor,
            record->descriptor.version_patch,
            digit_module_state_string(
                record->state
            )
        );


    if (
        written < 0 ||
        (size_t)written >=
            sizeof(message)
    )
    {
        return;
    }


    digit_modules_audit(
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
 * INITIALIZE MODULE INFRASTRUCTURE
 * ------------------------------------------------
 */

static int digit_modules_initialize_core(void)
{
    digit_module_result_t
        module_result;

    digit_module_inventory_result_t
        inventory_result;

    char message[
        DIGIT_MODULE_AUDIT_MESSAGE_MAX
    ];

    int written;


    digit_module_registry_init(
        &g_module_registry
    );


    digit_module_loader_init(
        &g_module_loader
    );


    digit_module_inventory_init(
        &g_module_inventory
    );


    memset(
        g_modules_path,
        0,
        sizeof(g_modules_path)
    );


    /*
     * Resolve:
     *
     *     <digit.exe directory>\modules
     */

    module_result =
        digit_module_discovery_get_path(
            g_modules_path,
            sizeof(g_modules_path)
        );


    if (
        module_result !=
        DIGIT_MODULE_OK
    )
    {
        fprintf(
            stderr,
            "[CORE] Module path resolution FAILED: %s\n",
            digit_module_result_string(
                module_result
            )
        );


        digit_modules_audit(
            "Module path resolution failed."
        );


        return 0;
    }


    /*
     * Module root is executable-relative and may
     * be created when absent.
     */

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


    /*
     * Persistent qualification inventory remains
     * under Digit's operational state directory.
     */

    if (
        !digit_modules_ensure_directory(
            DIGIT_MODULE_STATE_DIRECTORY
        )
    )
    {
        fprintf(
            stderr,
            "[CORE] Module state directory FAILED: %s\n",
            DIGIT_MODULE_STATE_DIRECTORY
        );


        digit_modules_audit(
            "Module state directory initialization failed."
        );


        return 0;
    }


    inventory_result =
        digit_module_inventory_configure(
            &g_module_inventory,
            DIGIT_MODULE_STATE_DIRECTORY
        );


    if (
        inventory_result !=
        DIGIT_MODULE_INVENTORY_OK
    )
    {
        fprintf(
            stderr,
            "[CORE] Module inventory configuration FAILED: %s\n",
            digit_module_inventory_result_string(
                inventory_result
            )
        );


        digit_modules_audit(
            "Module inventory configuration failed."
        );


        return 0;
    }


    inventory_result =
        digit_module_inventory_load(
            &g_module_inventory
        );


    if (
        inventory_result !=
        DIGIT_MODULE_INVENTORY_OK
    )
    {
        /*
         * Invalid persisted evidence is not reused.
         *
         * Reset the in-memory inventory so modules
         * must qualify again.
         */

        printf(
            "[CORE] Module inventory invalid: %s\n",
            digit_module_inventory_result_string(
                inventory_result
            )
        );


        digit_modules_audit(
            "Module inventory invalid; live qualification required."
        );


        digit_module_inventory_init(
            &g_module_inventory
        );


        inventory_result =
            digit_module_inventory_configure(
                &g_module_inventory,
                DIGIT_MODULE_STATE_DIRECTORY
            );


        if (
            inventory_result !=
            DIGIT_MODULE_INVENTORY_OK
        )
        {
            return 0;
        }
    }


    printf(
        "[CORE] Module inventory: %u record(s)\n",
        (unsigned int)
        g_module_inventory.count
    );


    written =
        snprintf(
            message,
            sizeof(message),
            "Module inventory loaded: records=%u",
            (unsigned int)
            g_module_inventory.count
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
 * DISCOVER MODULES
 * ------------------------------------------------
 */

static int digit_modules_discover(void)
{
    digit_module_discovery_report_t
        report;

    digit_module_result_t
        result;

    char message[
        DIGIT_MODULE_AUDIT_MESSAGE_MAX
    ];

    int written;


    memset(
        &report,
        0,
        sizeof(report)
    );


    result =
        digit_module_discovery_scan(
            &g_module_registry,
            &g_module_loader,
            g_modules_path,
            &report
        );


    if (
        result !=
        DIGIT_MODULE_OK
    )
    {
        fprintf(
            stderr,
            "[MODULE] Discovery FAILED: %s\n",
            digit_module_result_string(
                result
            )
        );


        written =
            snprintf(
                message,
                sizeof(message),
                "Module discovery failed: result=%s",
                digit_module_result_string(
                    result
                )
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


        return 0;
    }


    printf(
        "[MODULE] Discovery: "
        "%u directories, "
        "%u loaded, "
        "%u discovered, "
        "%u rejected\n",

        (unsigned int)
        report.directories_examined,

        (unsigned int)
        report.modules_loaded,

        (unsigned int)
        report.modules_discovered,

        (unsigned int)
        report.modules_rejected
    );


    written =
        snprintf(
            message,
            sizeof(message),

            "Module discovery complete: "
            "directories=%u loaded=%u "
            "discovered=%u rejected=%u",

            (unsigned int)
            report.directories_examined,

            (unsigned int)
            report.modules_loaded,

            (unsigned int)
            report.modules_discovered,

            (unsigned int)
            report.modules_rejected
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
 * PROCESS DISCOVERED MODULES
 * ------------------------------------------------
 */

static int digit_modules_process(void)
{
    size_t index;


    for (
        index = 0;
        index < g_module_registry.count;
        ++index
    )
    {
        digit_module_record_t
            *record;

        const digit_module_inventory_record_t
            *inventory_record;

        digit_module_result_t
            module_result;

        digit_module_inventory_result_t
            inventory_result;

        char message[
            DIGIT_MODULE_AUDIT_MESSAGE_MAX
        ];

        int written;

        int qualified =
            0;


        record =
            &g_module_registry.modules[index];


        /*
         * ------------------------------------------------
         * DISCOVERED
         * ------------------------------------------------
         */

        printf(
            "[MODULE] Discovered: %s (%s)\n",
            record->descriptor.name,
            record->descriptor.id
        );


        digit_modules_audit_module(
            "Module discovered",
            record
        );


        /*
         * ------------------------------------------------
         * VERIFY
         * ------------------------------------------------
         */

        module_result =
            digit_module_registry_verify(
                &g_module_registry,
                record->descriptor.id
            );


        if (
            module_result !=
            DIGIT_MODULE_OK
        )
        {
            printf(
                "[MODULE] Verification FAILED: %s (%s)\n",
                record->descriptor.id,
                digit_module_result_string(
                    module_result
                )
            );


            written =
                snprintf(
                    message,
                    sizeof(message),
                    "Module verification failed: id=%s result=%s",
                    record->descriptor.id,
                    digit_module_result_string(
                        module_result
                    )
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


            continue;
        }


        printf(
            "[MODULE] Verification PASS: %s\n",
            record->descriptor.id
        );


        /*
         * ------------------------------------------------
         * RESTORE QUALIFICATION
         * ------------------------------------------------
         */

        inventory_record =
            digit_module_inventory_find(
                &g_module_inventory,
                &record->descriptor
            );


        if (
            inventory_record != NULL
        )
        {
            module_result =
                digit_module_registry_restore_qualification(
                    &g_module_registry,
                    record->descriptor.id,
                    &inventory_record->qualification
                );


            if (
                module_result ==
                DIGIT_MODULE_OK
            )
            {
                printf(
                    "[MODULE] QUALIFIED: %s "
                    "(restored %u/%u, negative PASS)\n",

                    record->descriptor.id,

                    inventory_record
                        ->qualification
                        .tests_passed,

                    inventory_record
                        ->qualification
                        .tests_executed
                );


                written =
                    snprintf(
                        message,
                        sizeof(message),

                        "Module qualification restored: "
                        "id=%s tests=%u/%u negative=PASS",

                        record->descriptor.id,

                        inventory_record
                            ->qualification
                            .tests_passed,

                        inventory_record
                            ->qualification
                            .tests_executed
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


                qualified =
                    1;
            }
        }


        /*
         * ------------------------------------------------
         * LIVE QUALIFICATION
         * ------------------------------------------------
         */

        if (
            !qualified
        )
        {
            printf(
                "[MODULE] Qualification starting: %s\n",
                record->descriptor.id
            );


            written =
                snprintf(
                    message,
                    sizeof(message),
                    "Module qualification starting: id=%s",
                    record->descriptor.id
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


            module_result =
                digit_module_registry_qualify(
                    &g_module_registry,
                    record->descriptor.id
                );


            if (
                module_result !=
                DIGIT_MODULE_OK
            )
            {
                printf(
                    "[MODULE] Qualification FAILED: %s (%s)\n",
                    record->descriptor.id,
                    digit_module_result_string(
                        module_result
                    )
                );


                written =
                    snprintf(
                        message,
                        sizeof(message),

                        "Module qualification failed: "
                        "id=%s result=%s",

                        record->descriptor.id,

                        digit_module_result_string(
                            module_result
                        )
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


                continue;
            }


            printf(
                "[MODULE] Qualification PASS: %s (%u/%u)\n",

                record->descriptor.id,

                record
                    ->qualification
                    .tests_passed,

                record
                    ->qualification
                    .tests_executed
            );


            printf(
                "[MODULE] Negative validation: %s\n",

                record
                    ->qualification
                    .negative_test_passed
                    ? "PASS"
                    : "FAIL"
            );


            inventory_result =
                digit_module_inventory_store(
                    &g_module_inventory,
                    &record->descriptor,
                    &record->qualification
                );


            if (
                inventory_result !=
                DIGIT_MODULE_INVENTORY_OK
            )
            {
                fprintf(
                    stderr,
                    "[MODULE] Inventory write FAILED: %s (%s)\n",

                    record->descriptor.id,

                    digit_module_inventory_result_string(
                        inventory_result
                    )
                );


                (void)
                digit_module_registry_fail(
                    &g_module_registry,
                    record->descriptor.id
                );


                written =
                    snprintf(
                        message,
                        sizeof(message),

                        "Module inventory write failed: "
                        "id=%s result=%s",

                        record->descriptor.id,

                        digit_module_inventory_result_string(
                            inventory_result
                        )
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


                continue;
            }


            written =
                snprintf(
                    message,
                    sizeof(message),

                    "Module qualified: "
                    "id=%s tests=%u/%u "
                    "negative=PASS inventory=STORED",

                    record->descriptor.id,

                    record
                        ->qualification
                        .tests_passed,

                    record
                        ->qualification
                        .tests_executed
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


            qualified =
                1;
        }


        /*
         * ------------------------------------------------
         * AUTHORIZE ACTIVATION
         * ------------------------------------------------
         */

        if (
            qualified
        )
        {
            module_result =
                digit_module_registry_authorize_activation(
                    &g_module_registry,
                    record->descriptor.id
                );


            if (
                module_result !=
                DIGIT_MODULE_OK
            )
            {
                printf(
                    "[MODULE] Activation authorization FAILED: %s (%s)\n",

                    record->descriptor.id,

                    digit_module_result_string(
                        module_result
                    )
                );


                continue;
            }


            /*
             * ------------------------------------------------
             * START
             * ------------------------------------------------
             */

            if (
                record->descriptor.start ==
                NULL
            )
            {
                printf(
                    "[MODULE] Activation FAILED: %s (START_MISSING)\n",
                    record->descriptor.id
                );


                (void)
                digit_module_registry_fail(
                    &g_module_registry,
                    record->descriptor.id
                );


                digit_modules_audit(
                    "Module activation failed: start callback missing."
                );


                continue;
            }


            printf(
                "[MODULE] Activation starting: %s\n",
                record->descriptor.id
            );


            written =
                snprintf(
                    message,
                    sizeof(message),
                    "Module activation starting: id=%s",
                    record->descriptor.id
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


            module_result =
                record
                    ->descriptor
                    .start(
                        &g_module_host
                    );


            if (
                module_result !=
                DIGIT_MODULE_OK
            )
            {
                printf(
                    "[MODULE] Activation FAILED: %s (%s)\n",

                    record->descriptor.id,

                    digit_module_result_string(
                        module_result
                    )
                );


                (void)
                digit_module_registry_fail(
                    &g_module_registry,
                    record->descriptor.id
                );


                written =
                    snprintf(
                        message,
                        sizeof(message),

                        "Module activation failed: "
                        "id=%s result=%s",

                        record->descriptor.id,

                        digit_module_result_string(
                            module_result
                        )
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


                continue;
            }


            /*
             * Module callback succeeded.
             *
             * Record ACTIVE through the Core registry.
             */

            module_result =
                digit_module_registry_activate(
                    &g_module_registry,
                    record->descriptor.id
                );


            if (
                module_result !=
                DIGIT_MODULE_OK
            )
            {
                (void)
                record
                    ->descriptor
                    .stop();


                printf(
                    "[MODULE] Activation state FAILED: %s (%s)\n",

                    record->descriptor.id,

                    digit_module_result_string(
                        module_result
                    )
                );


                continue;
            }


            printf(
                "[MODULE] ACTIVE: %s\n",
                record->descriptor.id
            );


            digit_modules_audit_module(
                "Module active",
                record
            );
        }
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
        !digit_modules_initialize_core()
    )
    {
        return -1;
    }


    if (
        !digit_modules_discover()
    )
    {
        digit_module_loader_unload_all(
            &g_module_loader
        );

        return -1;
    }


    if (
        !digit_modules_process()
    )
    {
        digit_module_loader_unload_all(
            &g_module_loader
        );

        return -1;
    }


    g_modules_initialized =
        1;


    digit_modules_audit(
        "Core module subsystem initialized."
    );


    return 0;
}


/*
 * ------------------------------------------------
 * READY STATE
 * ------------------------------------------------
 */

int digit_modules_is_ready(void)
{
    return
        g_modules_initialized;
}


/*
 * ------------------------------------------------
 * PUBLIC MODULE EXPORT LOOKUP
 * ------------------------------------------------
 *
 * Resolve a module-specific export only from a DLL
 * already owned by the Core module loader.
 *
 * Capability access is denied unless:
 *
 * - the module subsystem is initialized;
 * - the module is registered;
 * - the module is ACTIVE;
 * - the loader still owns the module HMODULE; and
 * - the requested export exists.
 *
 * This function never loads or reloads a DLL.
 */

FARPROC digit_modules_get_export(
    const char *module_id,
    const char *export_name
)
{
    const digit_loaded_module_t
        *loaded;

    const digit_module_record_t
        *record =
            NULL;

    FARPROC export_address;

    size_t index;

    char message[
        DIGIT_MODULE_AUDIT_MESSAGE_MAX
    ];

    int written;


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


    /*
     * Require the module to be ACTIVE in the Core
     * registry before exposing any module-specific
     * capability.
     */

    for (
        index = 0;
        index < g_module_registry.count;
        ++index
    )
    {
        if (
            strcmp(
                g_module_registry
                    .modules[index]
                    .descriptor
                    .id,
                module_id
            ) == 0
        )
        {
            record =
                &g_module_registry
                    .modules[index];

            break;
        }
    }


    if (
        record == NULL ||
        record->state !=
            DIGIT_MODULE_STATE_ACTIVE
    )
    {
        written =
            snprintf(
                message,
                sizeof(message),
                "Module export denied: id=%s export=%s reason=NOT_ACTIVE",
                module_id,
                export_name
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


        return NULL;
    }


    /*
     * Use the HMODULE already retained by the
     * established Core loader.
     */

    loaded =
        digit_module_loader_find(
            &g_module_loader,
            module_id
        );


    if (
        loaded == NULL ||
        loaded->handle == NULL
    )
    {
        written =
            snprintf(
                message,
                sizeof(message),
                "Module export failed: id=%s export=%s reason=LOADER_RECORD_MISSING",
                module_id,
                export_name
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


        return NULL;
    }


    export_address =
        GetProcAddress(
            loaded->handle,
            export_name
        );


    if (
        export_address == NULL
    )
    {
        written =
            snprintf(
                message,
                sizeof(message),
                "Module export failed: id=%s export=%s reason=EXPORT_MISSING",
                module_id,
                export_name
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


        return NULL;
    }


    written =
        snprintf(
            message,
            sizeof(message),
            "Module export resolved: id=%s export=%s",
            module_id,
            export_name
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


    return
        export_address;
}


/*
 * ------------------------------------------------
 * PUBLIC SHUTDOWN
 * ------------------------------------------------
 */

void digit_modules_shutdown(void)
{
    size_t index;


    if (
        !g_modules_initialized
    )
    {
        return;
    }


    /*
     * Stop modules in reverse registration order.
     */

    index =
        g_module_registry.count;


    while (
        index > 0
    )
    {
        digit_module_record_t
            *record;

        digit_module_result_t
            result;

        char message[
            DIGIT_MODULE_AUDIT_MESSAGE_MAX
        ];

        int written;


        --index;


        record =
            &g_module_registry.modules[index];


        if (
            record->state !=
            DIGIT_MODULE_STATE_ACTIVE
        )
        {
            continue;
        }


        printf(
            "[MODULE] Stop starting: %s\n",
            record->descriptor.id
        );


        written =
            snprintf(
                message,
                sizeof(message),
                "Module stop starting: id=%s",
                record->descriptor.id
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


        if (
            record->descriptor.stop ==
            NULL
        )
        {
            printf(
                "[MODULE] Stop FAILED: %s (STOP_MISSING)\n",
                record->descriptor.id
            );


            continue;
        }


        result =
            record
                ->descriptor
                .stop();


        if (
            result !=
            DIGIT_MODULE_OK
        )
        {
            printf(
                "[MODULE] Stop FAILED: %s (%s)\n",

                record->descriptor.id,

                digit_module_result_string(
                    result
                )
            );


            written =
                snprintf(
                    message,
                    sizeof(message),

                    "Module stop failed: "
                    "id=%s result=%s",

                    record->descriptor.id,

                    digit_module_result_string(
                        result
                    )
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


            continue;
        }


        printf(
            "[MODULE] Stopped: %s\n",
            record->descriptor.id
        );


        written =
            snprintf(
                message,
                sizeof(message),
                "Module stopped: id=%s",
                record->descriptor.id
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
    }


    /*
     * Function pointers into module DLLs are no
     * longer used beyond this point.
     */

    digit_module_loader_unload_all(
        &g_module_loader
    );


    digit_modules_audit(
        "Core module subsystem shut down."
    );


    memset(
        &g_module_registry,
        0,
        sizeof(g_module_registry)
    );


    memset(
        &g_module_loader,
        0,
        sizeof(g_module_loader)
    );


    memset(
        &g_module_inventory,
        0,
        sizeof(g_module_inventory)
    );


    memset(
        g_modules_path,
        0,
        sizeof(g_modules_path)
    );


    g_modules_initialized =
        0;
}