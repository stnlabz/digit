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
 * - module directory monitoring;
 * - versioned replacement candidate staging;
 * - safe export-use leasing;
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
 * - controlled replacement lifecycle.
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

#define DIGIT_MODULE_VERSION_TEXT_MAX \
    32

#define DIGIT_MODULE_WATCH_BUFFER_SIZE \
    16384

#define DIGIT_MODULE_FILE_STABLE_CHECK_MS \
    100

#define DIGIT_MODULE_FILE_STABLE_COUNT \
    3


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
 * MODULE USE STATE
 * ------------------------------------------------
 */

typedef struct
{
    char module_id[
        STNLABZ_MODULE_ID_MAX
    ];

    unsigned long active_calls;

    int replacement_pending;

} digit_module_use_state_t;


static digit_module_use_state_t
    g_module_use[
        STNLABZ_MODULE_LOADER_MAX
    ];

static CRITICAL_SECTION
    g_module_lock;

static int g_module_lock_initialized =
    0;


/*
 * ------------------------------------------------
 * WATCHER STATE
 * ------------------------------------------------
 */

static HANDLE g_module_watcher_thread =
    NULL;

static HANDLE g_module_watcher_stop_event =
    NULL;


/*
 * ------------------------------------------------
 * HOST SERVICES
 * ------------------------------------------------
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
            ) != 0 ||
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
 * VERSION
 * ------------------------------------------------
 */

static int digit_modules_parse_version(
    const char *version,
    unsigned int *major_out,
    unsigned int *minor_out,
    unsigned int *patch_out
)
{
    unsigned int major;

    unsigned int minor;

    unsigned int patch;

    char extra;

    int fields;


    if (
        version == NULL ||
        version[0] == '\0' ||
        major_out == NULL ||
        minor_out == NULL ||
        patch_out == NULL
    )
    {
        return 0;
    }


    fields =
        sscanf_s(
            version,
            "%u.%u.%u%c",
            &major,
            &minor,
            &patch,
            &extra,
            (unsigned int)sizeof(extra)
        );


    if (
        fields != 3
    )
    {
        return 0;
    }


    *major_out =
        major;

    *minor_out =
        minor;

    *patch_out =
        patch;


    return 1;
}


static int digit_modules_version_newer(
    unsigned int candidate_major,
    unsigned int candidate_minor,
    unsigned int candidate_patch,
    unsigned int current_major,
    unsigned int current_minor,
    unsigned int current_patch
)
{
    if (
        candidate_major !=
        current_major
    )
    {
        return
            candidate_major >
            current_major;
    }


    if (
        candidate_minor !=
        current_minor
    )
    {
        return
            candidate_minor >
            current_minor;
    }


    return
        candidate_patch >
        current_patch;
}


/*
 * ------------------------------------------------
 * USE STATE
 * ------------------------------------------------
 */

static digit_module_use_state_t *
digit_modules_use_state_find_locked(
    const char *module_id,
    int create
)
{
    size_t index;

    digit_module_use_state_t
        *empty =
            NULL;


    for (
        index = 0U;
        index <
            STNLABZ_MODULE_LOADER_MAX;
        ++index
    )
    {
        if (
            g_module_use[index]
                .module_id[0] == '\0'
        )
        {
            if (
                empty == NULL
            )
            {
                empty =
                    &g_module_use[index];
            }


            continue;
        }


        if (
            strcmp(
                g_module_use[index].module_id,
                module_id
            ) == 0
        )
        {
            return
                &g_module_use[index];
        }
    }


    if (
        !create ||
        empty == NULL
    )
    {
        return NULL;
    }


    if (
        strcpy_s(
            empty->module_id,
            sizeof(empty->module_id),
            module_id
        ) != 0
    )
    {
        return NULL;
    }


    empty->active_calls =
        0UL;

    empty->replacement_pending =
        0;


    return empty;
}


/*
 * ------------------------------------------------
 * EXPORT LEASE
 * ------------------------------------------------
 */

int digit_modules_acquire_export(
    const char *module_id,
    const char *export_name,
    FARPROC *export_out
)
{
    const stnlabz_module_record_t
        *record;

    const stnlabz_loaded_module_t
        *loaded;

    digit_module_use_state_t
        *use_state;

    FARPROC export_address;


    if (
        module_id == NULL ||
        export_name == NULL ||
        export_out == NULL ||
        module_id[0] == '\0' ||
        export_name[0] == '\0'
    )
    {
        return -1;
    }


    *export_out =
        NULL;


    if (
        !g_module_lock_initialized
    )
    {
        return -1;
    }


    EnterCriticalSection(
        &g_module_lock
    );


    if (
        !g_modules_initialized
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    use_state =
        digit_modules_use_state_find_locked(
            module_id,
            1
        );


    if (
        use_state == NULL ||
        use_state->replacement_pending
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
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
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
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
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
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
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    use_state->active_calls++;


    *export_out =
        export_address;


    LeaveCriticalSection(
        &g_module_lock
    );


    return 0;
}


void digit_modules_release_export(
    const char *module_id
)
{
    digit_module_use_state_t
        *use_state;


    if (
        module_id == NULL ||
        module_id[0] == '\0' ||
        !g_module_lock_initialized
    )
    {
        return;
    }


    EnterCriticalSection(
        &g_module_lock
    );


    use_state =
        digit_modules_use_state_find_locked(
            module_id,
            0
        );


    if (
        use_state != NULL &&
        use_state->active_calls > 0UL
    )
    {
        use_state->active_calls--;
    }


    LeaveCriticalSection(
        &g_module_lock
    );
}


/*
 * ------------------------------------------------
 * FILE STABILITY
 * ------------------------------------------------
 */

static int digit_modules_wait_file_stable(
    const char *path
)
{
    LARGE_INTEGER previous_size;

    FILETIME previous_write;

    int previous_valid =
        0;

    unsigned int stable_count =
        0U;


    memset(
        &previous_size,
        0,
        sizeof(previous_size)
    );


    memset(
        &previous_write,
        0,
        sizeof(previous_write)
    );


    for (;;)
    {
        HANDLE file;

        LARGE_INTEGER current_size;

        FILETIME current_write;


        if (
            g_module_watcher_stop_event !=
                NULL &&
            WaitForSingleObject(
                g_module_watcher_stop_event,
                0
            ) ==
                WAIT_OBJECT_0
        )
        {
            return 0;
        }


        file =
            CreateFileA(
                path,
                GENERIC_READ,
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );


        if (
            file ==
            INVALID_HANDLE_VALUE
        )
        {
            stable_count =
                0U;

            previous_valid =
                0;


            Sleep(
                DIGIT_MODULE_FILE_STABLE_CHECK_MS
            );


            continue;
        }


        if (
            !GetFileSizeEx(
                file,
                &current_size
            ) ||
            !GetFileTime(
                file,
                NULL,
                NULL,
                &current_write
            )
        )
        {
            CloseHandle(
                file
            );


            stable_count =
                0U;

            previous_valid =
                0;


            Sleep(
                DIGIT_MODULE_FILE_STABLE_CHECK_MS
            );


            continue;
        }


        CloseHandle(
            file
        );


        if (
            previous_valid &&
            current_size.QuadPart ==
                previous_size.QuadPart &&
            CompareFileTime(
                &current_write,
                &previous_write
            ) == 0
        )
        {
            stable_count++;


            if (
                stable_count >=
                    DIGIT_MODULE_FILE_STABLE_COUNT
            )
            {
                return 1;
            }
        }
        else
        {
            stable_count =
                0U;
        }


        previous_size =
            current_size;

        previous_write =
            current_write;

        previous_valid =
            1;


        Sleep(
            DIGIT_MODULE_FILE_STABLE_CHECK_MS
        );
    }
}


/*
 * ------------------------------------------------
 * CANDIDATE PREFLIGHT
 * ------------------------------------------------
 */

static int digit_modules_preflight_candidate(
    const char *module_id,
    const char *candidate_path,
    unsigned int expected_major,
    unsigned int expected_minor,
    unsigned int expected_patch
)
{
    stnlabz_module_loader_t
        candidate_loader;

    stnlabz_module_registry_t
        candidate_registry;

    const stnlabz_module_descriptor_t
        *descriptor =
            NULL;

    const stnlabz_module_record_t
        *record;

    stnlabz_module_loader_result_t
        loader_result;

    stnlabz_module_result_t
        abi_result;


    stnlabz_module_loader_init(
        &candidate_loader
    );


    stnlabz_module_registry_init(
        &candidate_registry
    );


    loader_result =
        stnlabz_module_loader_load(
            &candidate_loader,
            module_id,
            candidate_path,
            &descriptor
        );


    if (
        loader_result !=
        STNLABZ_MODULE_LOADER_OK
    )
    {
        printf(
            "[MODULE] Candidate loader FAILED: "
            "%s (%s)\n",
            module_id,
            stnlabz_module_loader_result_string(
                loader_result
            )
        );


        return 0;
    }


    if (
        descriptor == NULL ||
        descriptor->version_major !=
            expected_major ||
        descriptor->version_minor !=
            expected_minor ||
        descriptor->version_patch !=
            expected_patch
    )
    {
        printf(
            "[MODULE] Candidate version mismatch: %s\n",
            module_id
        );


        (void)
        stnlabz_module_loader_unload(
            &candidate_loader,
            module_id
        );


        return 0;
    }


    abi_result =
        stnlabz_module_abi_prepare(
            &candidate_registry,
            descriptor
        );


    if (
        abi_result !=
        STNLABZ_MODULE_OK
    )
    {
        printf(
            "[MODULE] Candidate ABI preflight FAILED: "
            "%s (%s)\n",
            module_id,
            stnlabz_module_result_string(
                abi_result
            )
        );


        (void)
        stnlabz_module_loader_unload(
            &candidate_loader,
            module_id
        );


        return 0;
    }


    record =
        stnlabz_module_registry_find(
            &candidate_registry,
            module_id
        );


    if (
        record == NULL ||
        record->state !=
            STNLABZ_MODULE_STATE_QUALIFIED ||
        record->qualification.tests_executed <
            STNLABZ_MODULE_MIN_TESTS ||
        record->qualification.tests_failed !=
            0U ||
        !record
            ->qualification
            .negative_test_executed ||
        !record
            ->qualification
            .negative_test_passed
    )
    {
        printf(
            "[MODULE] Candidate qualification "
            "evidence FAILED: %s\n",
            module_id
        );


        (void)
        stnlabz_module_loader_unload(
            &candidate_loader,
            module_id
        );


        return 0;
    }


    abi_result =
        stnlabz_module_abi_unregister(
            &candidate_registry,
            module_id
        );


    if (
        abi_result !=
        STNLABZ_MODULE_OK
    )
    {
        printf(
            "[MODULE] Candidate preflight unregister "
            "FAILED: %s (%s)\n",
            module_id,
            stnlabz_module_result_string(
                abi_result
            )
        );


        return 0;
    }


    loader_result =
        stnlabz_module_loader_unload(
            &candidate_loader,
            module_id
        );


    if (
        loader_result !=
        STNLABZ_MODULE_LOADER_OK
    )
    {
        printf(
            "[MODULE] Candidate preflight unload "
            "FAILED: %s (%s)\n",
            module_id,
            stnlabz_module_loader_result_string(
                loader_result
            )
        );


        return 0;
    }


    return 1;
}


/*
 * ------------------------------------------------
 * MODULE DIRECTORY VALIDATION
 * ------------------------------------------------
 */

static int digit_modules_validate_directory(
    const char *module_id,
    const char *directory_path
)
{
    char config_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char declared_id[
        STNLABZ_MODULE_ID_MAX
    ];


    if (
        !digit_modules_join_path(
            config_path,
            sizeof(config_path),
            directory_path,
            DIGIT_MODULE_CONF_NAME
        )
    )
    {
        return 0;
    }


    if (
        !digit_modules_read_id(
            config_path,
            declared_id,
            sizeof(declared_id)
        )
    )
    {
        return 0;
    }


    return
        strcmp(
            module_id,
            declared_id
        ) == 0;
}


/*
 * ------------------------------------------------
 * NEW MODULE
 * ------------------------------------------------
 */

static int digit_modules_load_new(
    const char *module_id,
    const char *dll_path
)
{
    const stnlabz_module_descriptor_t
        *descriptor =
            NULL;

    const stnlabz_module_record_t
        *record;

    stnlabz_module_loader_result_t
        loader_result;

    stnlabz_module_result_t
        abi_result;

    digit_module_use_state_t
        *use_state;


    EnterCriticalSection(
        &g_module_lock
    );


    record =
        stnlabz_module_registry_find(
            &g_module_registry,
            module_id
        );


    if (
        record != NULL
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return 0;
    }


    printf(
        "[MODULE] New module detected: %s\n",
        module_id
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
        printf(
            "[MODULE] New module REJECTED: %s "
            "(LOADER_%s)\n",
            module_id,
            stnlabz_module_loader_result_string(
                loader_result
            )
        );


        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    abi_result =
        stnlabz_module_abi_prepare(
            &g_module_registry,
            descriptor
        );


    if (
        abi_result !=
        STNLABZ_MODULE_OK
    )
    {
        printf(
            "[MODULE] New module REJECTED: %s "
            "(ABI_PREPARE_%s)\n",
            module_id,
            stnlabz_module_result_string(
                abi_result
            )
        );


        (void)
        stnlabz_module_loader_unload(
            &g_module_loader,
            module_id
        );


        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    record =
        stnlabz_module_registry_find(
            &g_module_registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    printf(
        "[MODULE] Verification PASS: %s\n",
        module_id
    );


    printf(
        "[MODULE] Qualification PASS: %s (%u/%u)\n",
        module_id,
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


    abi_result =
        stnlabz_module_abi_authorize_and_activate(
            &g_module_registry,
            module_id,
            &g_module_host
        );


    if (
        abi_result !=
        STNLABZ_MODULE_OK
    )
    {
        printf(
            "[MODULE] New module activation FAILED: "
            "%s (%s)\n",
            module_id,
            stnlabz_module_result_string(
                abi_result
            )
        );


        (void)
        stnlabz_module_abi_unregister(
            &g_module_registry,
            module_id
        );


        (void)
        stnlabz_module_loader_unload(
            &g_module_loader,
            module_id
        );


        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    use_state =
        digit_modules_use_state_find_locked(
            module_id,
            1
        );


    if (
        use_state == NULL
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    printf(
        "[MODULE] ACTIVE: %s %u.%u.%u\n",
        module_id,
        descriptor->version_major,
        descriptor->version_minor,
        descriptor->version_patch
    );


    LeaveCriticalSection(
        &g_module_lock
    );


    return 0;
}


/*
 * ------------------------------------------------
 * WAIT FOR MODULE QUIESCENCE
 * ------------------------------------------------
 */

static int digit_modules_wait_quiescent(
    const char *module_id
)
{
    for (;;)
    {
        digit_module_use_state_t
            *use_state;

        unsigned long active_calls;


        if (
            g_module_watcher_stop_event !=
                NULL &&
            WaitForSingleObject(
                g_module_watcher_stop_event,
                0
            ) ==
                WAIT_OBJECT_0
        )
        {
            return 0;
        }


        EnterCriticalSection(
            &g_module_lock
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        active_calls =
            (
                use_state != NULL
            )
            ? use_state->active_calls
            : 0UL;


        LeaveCriticalSection(
            &g_module_lock
        );


        if (
            active_calls == 0UL
        )
        {
            return 1;
        }


        Sleep(
            10
        );
    }
}


/*
 * ------------------------------------------------
 * HOT REPLACEMENT
 * ------------------------------------------------
 */

static int digit_modules_replace(
    const char *module_id,
    const char *candidate_path,
    unsigned int candidate_major,
    unsigned int candidate_minor,
    unsigned int candidate_patch
)
{
    const stnlabz_module_record_t
        *record;

    const stnlabz_module_descriptor_t
        *descriptor =
            NULL;

    digit_module_use_state_t
        *use_state;

    stnlabz_module_loader_result_t
        loader_result;

    stnlabz_module_result_t
        abi_result;

    char directory_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char canonical_name[
        STNLABZ_MODULE_ID_MAX + 16U
    ];

    char canonical_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char audit_message[
        DIGIT_MODULE_AUDIT_MESSAGE_MAX
    ];

    unsigned int old_major;

    unsigned int old_minor;

    unsigned int old_patch;

    int written;


    EnterCriticalSection(
        &g_module_lock
    );


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
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    old_major =
        record->descriptor.version_major;

    old_minor =
        record->descriptor.version_minor;

    old_patch =
        record->descriptor.version_patch;


    if (
        !digit_modules_version_newer(
            candidate_major,
            candidate_minor,
            candidate_patch,
            old_major,
            old_minor,
            old_patch
        )
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return 0;
    }


    use_state =
        digit_modules_use_state_find_locked(
            module_id,
            1
        );


    if (
        use_state == NULL ||
        use_state->replacement_pending
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    use_state->replacement_pending =
        1;


    LeaveCriticalSection(
        &g_module_lock
    );


    printf(
        "[MODULE] Update detected: %s "
        "%u.%u.%u -> %u.%u.%u\n",
        module_id,
        old_major,
        old_minor,
        old_patch,
        candidate_major,
        candidate_minor,
        candidate_patch
    );


    printf(
        "[MODULE] Candidate preflight starting: %s\n",
        module_id
    );


    if (
        !digit_modules_preflight_candidate(
            module_id,
            candidate_path,
            candidate_major,
            candidate_minor,
            candidate_patch
        )
    )
    {
        EnterCriticalSection(
            &g_module_lock
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        if (
            use_state != NULL
        )
        {
            use_state->replacement_pending =
                0;
        }


        LeaveCriticalSection(
            &g_module_lock
        );


        printf(
            "[MODULE] Update REJECTED: %s "
            "(CANDIDATE_PREFLIGHT_FAILED)\n",
            module_id
        );


        return -1;
    }


    printf(
        "[MODULE] Candidate preflight PASS: "
        "%s %u.%u.%u\n",
        module_id,
        candidate_major,
        candidate_minor,
        candidate_patch
    );


    /*
     * No new calls may enter once replacement_pending
     * is set.
     *
     * Existing users drain naturally.
     */

    printf(
        "[MODULE] Waiting for active calls to drain: %s\n",
        module_id
    );


    if (
        !digit_modules_wait_quiescent(
            module_id
        )
    )
    {
        return -1;
    }


    printf(
        "[MODULE] Module quiescent: %s\n",
        module_id
    );


    EnterCriticalSection(
        &g_module_lock
    );


    /*
     * ABI 1.4 owns:
     *
     * ACTIVE -> STOPPED -> UNREGISTERED
     */

    abi_result =
        stnlabz_module_abi_prepare_replacement(
            &g_module_registry,
            module_id
        );


    if (
        abi_result !=
        STNLABZ_MODULE_OK
    )
    {
        printf(
            "[MODULE] Update FAILED: %s "
            "(ABI_PREPARE_REPLACEMENT_%s)\n",
            module_id,
            stnlabz_module_result_string(
                abi_result
            )
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        if (
            use_state != NULL
        )
        {
            use_state->replacement_pending =
                0;
        }


        LeaveCriticalSection(
            &g_module_lock
        );


        return -1;
    }


    printf(
        "[MODULE] STOPPED + UNREGISTERED: %s\n",
        module_id
    );


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
        printf(
            "[MODULE] Update FAILED: %s "
            "(LOADER_UNLOAD_%s)\n",
            module_id,
            stnlabz_module_loader_result_string(
                loader_result
            )
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        if (
            use_state != NULL
        )
        {
            use_state->replacement_pending =
                0;
        }


        LeaveCriticalSection(
            &g_module_lock
        );


        return -1;
    }


    printf(
        "[MODULE] DLL unloaded: %s\n",
        module_id
    );


    written =
        snprintf(
            directory_path,
            sizeof(directory_path),
            "%s\\%s",
            g_modules_path,
            module_id
        );


    if (
        written < 0 ||
        (size_t)written >=
            sizeof(directory_path)
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    written =
        snprintf(
            canonical_name,
            sizeof(canonical_name),
            "digit_%s.dll",
            module_id
        );


    if (
        written < 0 ||
        (size_t)written >=
            sizeof(canonical_name)
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    if (
        !digit_modules_join_path(
            canonical_path,
            sizeof(canonical_path),
            directory_path,
            canonical_name
        )
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    if (
        !CopyFileA(
            candidate_path,
            canonical_path,
            FALSE
        )
    )
    {
        printf(
            "[MODULE] Update FAILED: %s "
            "(INSTALL_FAILED:%lu)\n",
            module_id,
            (unsigned long)GetLastError()
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        if (
            use_state != NULL
        )
        {
            use_state->replacement_pending =
                0;
        }


        LeaveCriticalSection(
            &g_module_lock
        );


        return -1;
    }


    printf(
        "[MODULE] Replacement installed: "
        "%s %u.%u.%u\n",
        module_id,
        candidate_major,
        candidate_minor,
        candidate_patch
    );


    loader_result =
        stnlabz_module_loader_load(
            &g_module_loader,
            module_id,
            canonical_path,
            &descriptor
        );


    if (
        loader_result !=
        STNLABZ_MODULE_LOADER_OK
    )
    {
        printf(
            "[MODULE] Update FAILED: %s "
            "(LOADER_LOAD_%s)\n",
            module_id,
            stnlabz_module_loader_result_string(
                loader_result
            )
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        if (
            use_state != NULL
        )
        {
            use_state->replacement_pending =
                0;
        }


        LeaveCriticalSection(
            &g_module_lock
        );


        return -1;
    }


    abi_result =
        stnlabz_module_abi_prepare(
            &g_module_registry,
            descriptor
        );


    if (
        abi_result !=
        STNLABZ_MODULE_OK
    )
    {
        printf(
            "[MODULE] Update FAILED: %s "
            "(ABI_PREPARE_%s)\n",
            module_id,
            stnlabz_module_result_string(
                abi_result
            )
        );


        (void)
        stnlabz_module_loader_unload(
            &g_module_loader,
            module_id
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        if (
            use_state != NULL
        )
        {
            use_state->replacement_pending =
                0;
        }


        LeaveCriticalSection(
            &g_module_lock
        );


        return -1;
    }


    record =
        stnlabz_module_registry_find(
            &g_module_registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return -1;
    }


    printf(
        "[MODULE] Verification PASS: %s\n",
        module_id
    );


    printf(
        "[MODULE] Qualification PASS: %s (%u/%u)\n",
        module_id,
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


    abi_result =
        stnlabz_module_abi_authorize_and_activate(
            &g_module_registry,
            module_id,
            &g_module_host
        );


    if (
        abi_result !=
        STNLABZ_MODULE_OK
    )
    {
        printf(
            "[MODULE] Update FAILED: %s "
            "(ACTIVATION_%s)\n",
            module_id,
            stnlabz_module_result_string(
                abi_result
            )
        );


        use_state =
            digit_modules_use_state_find_locked(
                module_id,
                0
            );


        if (
            use_state != NULL
        )
        {
            use_state->replacement_pending =
                0;
        }


        LeaveCriticalSection(
            &g_module_lock
        );


        return -1;
    }


    use_state =
        digit_modules_use_state_find_locked(
            module_id,
            0
        );


    if (
        use_state != NULL
    )
    {
        use_state->replacement_pending =
            0;
    }


    printf(
        "[MODULE] ACTIVE: %s %u.%u.%u\n",
        module_id,
        candidate_major,
        candidate_minor,
        candidate_patch
    );


    printf(
        "[MODULE] Hot replacement PASS: %s "
        "%u.%u.%u -> %u.%u.%u\n",
        module_id,
        old_major,
        old_minor,
        old_patch,
        candidate_major,
        candidate_minor,
        candidate_patch
    );


    written =
        snprintf(
            audit_message,
            sizeof(audit_message),
            "Hot replacement PASS: "
            "module=%s old=%u.%u.%u new=%u.%u.%u",
            module_id,
            old_major,
            old_minor,
            old_patch,
            candidate_major,
            candidate_minor,
            candidate_patch
        );


    if (
        written >= 0 &&
        (size_t)written <
            sizeof(audit_message)
    )
    {
        digit_modules_audit(
            audit_message
        );
    }


    LeaveCriticalSection(
        &g_module_lock
    );


    return 0;
}


/*
 * ------------------------------------------------
 * WATCH EVENT
 * ------------------------------------------------
 *
 * Supported deployment forms:
 *
 * Existing module update:
 *
 *     modules\<id>\digit_<id>.<version>.dll
 *
 * New module:
 *
 *     modules\<id>\module.conf
 *     modules\<id>\digit_<id>.dll
 *
 * A versioned DLL may also introduce a new module.
 */

static void digit_modules_handle_watch_path(
    const char *relative_path
)
{
    const char *separator;

    const char *filename;

    char module_id[
        STNLABZ_MODULE_ID_MAX
    ];

    char directory_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char full_path[
        DIGIT_MODULE_PATH_MAX
    ];

    char canonical_name[
        STNLABZ_MODULE_ID_MAX + 16U
    ];

    char versioned_prefix[
        STNLABZ_MODULE_ID_MAX + 16U
    ];

    size_t module_id_length;

    size_t prefix_length;

    size_t filename_length;

    const char *version_start;

    size_t version_length;

    char version[
        DIGIT_MODULE_VERSION_TEXT_MAX
    ];

    unsigned int version_major;

    unsigned int version_minor;

    unsigned int version_patch;

    const stnlabz_module_record_t
        *record;


    if (
        relative_path == NULL ||
        relative_path[0] == '\0'
    )
    {
        return;
    }


    separator =
        strchr(
            relative_path,
            '\\'
        );


    if (
        separator == NULL
    )
    {
        return;
    }


    /*
     * Only one module-directory level is accepted.
     */

    if (
        strchr(
            separator + 1,
            '\\'
        ) != NULL
    )
    {
        return;
    }


    module_id_length =
        (size_t)(
            separator -
            relative_path
        );


    if (
        module_id_length == 0U ||
        module_id_length >=
            sizeof(module_id)
    )
    {
        return;
    }


    memcpy(
        module_id,
        relative_path,
        module_id_length
    );


    module_id[
        module_id_length
    ] =
        '\0';


    filename =
        separator + 1;


    if (
        filename[0] == '\0'
    )
    {
        return;
    }


    if (
        snprintf(
            directory_path,
            sizeof(directory_path),
            "%s\\%s",
            g_modules_path,
            module_id
        ) < 0
    )
    {
        return;
    }


    if (
        !digit_modules_validate_directory(
            module_id,
            directory_path
        )
    )
    {
        return;
    }


    if (
        !digit_modules_join_path(
            full_path,
            sizeof(full_path),
            g_modules_path,
            relative_path
        )
    )
    {
        return;
    }


    if (
        strcmp(
            filename,
            DIGIT_MODULE_CONF_NAME
        ) == 0
    )
    {
        /*
         * Configuration changes are observed but the DLL
         * event is the activation trigger.
         */

        return;
    }


    if (
        snprintf(
            canonical_name,
            sizeof(canonical_name),
            "digit_%s.dll",
            module_id
        ) < 0
    )
    {
        return;
    }


    /*
     * Canonical DLL:
     *
     * If module is not already known, this is a new
     * module candidate.
     */

    if (
        strcmp(
            filename,
            canonical_name
        ) == 0
    )
    {
        EnterCriticalSection(
            &g_module_lock
        );


        record =
            stnlabz_module_registry_find(
                &g_module_registry,
                module_id
            );


        LeaveCriticalSection(
            &g_module_lock
        );


        if (
            record != NULL
        )
        {
            /*
             * Existing Windows modules update through
             * side-by-side versioned candidates so the
             * active mapped DLL is never overwritten.
             */

            return;
        }


        if (
            !digit_modules_wait_file_stable(
                full_path
            )
        )
        {
            return;
        }


        (void)
        digit_modules_load_new(
            module_id,
            full_path
        );


        return;
    }


    /*
     * Versioned DLL:
     *
     *     digit_<id>.<major>.<minor>.<patch>.dll
     */

    if (
        snprintf(
            versioned_prefix,
            sizeof(versioned_prefix),
            "digit_%s.",
            module_id
        ) < 0
    )
    {
        return;
    }


    prefix_length =
        strlen(
            versioned_prefix
        );


    filename_length =
        strlen(
            filename
        );


    if (
        filename_length <=
            prefix_length + 4U ||
        strncmp(
            filename,
            versioned_prefix,
            prefix_length
        ) != 0 ||
        strcmp(
            filename +
                filename_length -
                4U,
            ".dll"
        ) != 0
    )
    {
        return;
    }


    version_start =
        filename +
        prefix_length;


    version_length =
        filename_length -
        prefix_length -
        4U;


    if (
        version_length == 0U ||
        version_length >=
            sizeof(version)
    )
    {
        return;
    }


    memcpy(
        version,
        version_start,
        version_length
    );


    version[
        version_length
    ] =
        '\0';


    if (
        !digit_modules_parse_version(
            version,
            &version_major,
            &version_minor,
            &version_patch
        )
    )
    {
        return;
    }


    if (
        !digit_modules_wait_file_stable(
            full_path
        )
    )
    {
        return;
    }


    EnterCriticalSection(
        &g_module_lock
    );


    record =
        stnlabz_module_registry_find(
            &g_module_registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );


        /*
         * New module supplied as a versioned candidate.
         *
         * Qualify it before installing the canonical
         * runtime artifact.
         */

        printf(
            "[MODULE] New versioned module candidate: "
            "%s %u.%u.%u\n",
            module_id,
            version_major,
            version_minor,
            version_patch
        );


        if (
            !digit_modules_preflight_candidate(
                module_id,
                full_path,
                version_major,
                version_minor,
                version_patch
            )
        )
        {
            printf(
                "[MODULE] New module REJECTED: %s "
                "(PREFLIGHT_FAILED)\n",
                module_id
            );


            return;
        }


        {
            char canonical_path[
                DIGIT_MODULE_PATH_MAX
            ];


            if (
                !digit_modules_join_path(
                    canonical_path,
                    sizeof(canonical_path),
                    directory_path,
                    canonical_name
                )
            )
            {
                return;
            }


            if (
                !CopyFileA(
                    full_path,
                    canonical_path,
                    FALSE
                )
            )
            {
                printf(
                    "[MODULE] New module install FAILED: "
                    "%s (%lu)\n",
                    module_id,
                    (unsigned long)GetLastError()
                );


                return;
            }


            (void)
            digit_modules_load_new(
                module_id,
                canonical_path
            );
        }


        return;
    }


    if (
        record->state !=
            STNLABZ_MODULE_STATE_ACTIVE
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return;
    }


    if (
        !digit_modules_version_newer(
            version_major,
            version_minor,
            version_patch,
            record->descriptor.version_major,
            record->descriptor.version_minor,
            record->descriptor.version_patch
        )
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );

        return;
    }


    LeaveCriticalSection(
        &g_module_lock
    );


    (void)
    digit_modules_replace(
        module_id,
        full_path,
        version_major,
        version_minor,
        version_patch
    );
}


/*
 * ------------------------------------------------
 * DIRECTORY WATCHER
 * ------------------------------------------------
 */

static DWORD WINAPI digit_modules_watcher_main(
    LPVOID context
)
{
    HANDLE directory;

    HANDLE change_event;

    HANDLE wait_handles[2];

    OVERLAPPED overlapped;

    BYTE buffer[
        DIGIT_MODULE_WATCH_BUFFER_SIZE
    ];

    int request_pending =
        0;


    (void)context;


    directory =
        CreateFileA(
            g_modules_path,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ |
            FILE_SHARE_WRITE |
            FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS |
            FILE_FLAG_OVERLAPPED,
            NULL
        );


    if (
        directory ==
        INVALID_HANDLE_VALUE
    )
    {
        printf(
            "[MODULE] Watcher FAILED: directory open (%lu)\n",
            (unsigned long)GetLastError()
        );


        return 1;
    }


    change_event =
        CreateEventA(
            NULL,
            TRUE,
            FALSE,
            NULL
        );


    if (
        change_event == NULL
    )
    {
        CloseHandle(
            directory
        );


        return 1;
    }


    wait_handles[0] =
        g_module_watcher_stop_event;

    wait_handles[1] =
        change_event;


    memset(
        &overlapped,
        0,
        sizeof(overlapped)
    );


    overlapped.hEvent =
        change_event;


    printf(
        "[MODULE] Watcher ACTIVE: %s\n",
        g_modules_path
    );


    for (;;)
    {
        DWORD wait_result;

        DWORD bytes_transferred;


        if (
            !request_pending
        )
        {
            ResetEvent(
                change_event
            );


            memset(
                buffer,
                0,
                sizeof(buffer)
            );


            if (
                !ReadDirectoryChangesW(
                    directory,
                    buffer,
                    (DWORD)sizeof(buffer),
                    TRUE,
                    FILE_NOTIFY_CHANGE_FILE_NAME |
                    FILE_NOTIFY_CHANGE_LAST_WRITE |
                    FILE_NOTIFY_CHANGE_SIZE,
                    NULL,
                    &overlapped,
                    NULL
                )
            )
            {
                printf(
                    "[MODULE] Watcher FAILED: "
                    "ReadDirectoryChangesW (%lu)\n",
                    (unsigned long)GetLastError()
                );


                break;
            }


            request_pending =
                1;
        }


        wait_result =
            WaitForMultipleObjects(
                2,
                wait_handles,
                FALSE,
                INFINITE
            );


        if (
            wait_result ==
            WAIT_OBJECT_0
        )
        {
            (void)
            CancelIoEx(
                directory,
                &overlapped
            );


            break;
        }


        if (
            wait_result !=
            WAIT_OBJECT_0 + 1
        )
        {
            break;
        }


        bytes_transferred =
            0U;


        if (
            !GetOverlappedResult(
                directory,
                &overlapped,
                &bytes_transferred,
                FALSE
            )
        )
        {
            DWORD error =
                GetLastError();


            if (
                error ==
                ERROR_OPERATION_ABORTED
            )
            {
                break;
            }


            printf(
                "[MODULE] Watcher FAILED: "
                "GetOverlappedResult (%lu)\n",
                (unsigned long)error
            );


            break;
        }


        request_pending =
            0;


        if (
            bytes_transferred > 0U
        )
        {
            FILE_NOTIFY_INFORMATION
                *notification;


            notification =
                (FILE_NOTIFY_INFORMATION *)
                buffer;


            for (;;)
            {
                char relative_path[
                    DIGIT_MODULE_PATH_MAX
                ];

                int converted;

                int characters;


                characters =
                    (int)(
                        notification
                            ->FileNameLength /
                        sizeof(WCHAR)
                    );


                if (
                    characters > 0 &&
                    characters <
                        DIGIT_MODULE_PATH_MAX
                )
                {
                    converted =
                        WideCharToMultiByte(
                            CP_ACP,
                            0,
                            notification->FileName,
                            characters,
                            relative_path,
                            DIGIT_MODULE_PATH_MAX - 1,
                            NULL,
                            NULL
                        );


                    if (
                        converted > 0
                    )
                    {
                        relative_path[
                            converted
                        ] =
                            '\0';


                        switch (
                            notification->Action
                        )
                        {
                            case FILE_ACTION_ADDED:

                            case FILE_ACTION_MODIFIED:

                            case FILE_ACTION_RENAMED_NEW_NAME:

                                digit_modules_handle_watch_path(
                                    relative_path
                                );

                                break;


                            default:

                                break;
                        }
                    }
                }


                if (
                    notification->NextEntryOffset ==
                    0U
                )
                {
                    break;
                }


                notification =
                    (FILE_NOTIFY_INFORMATION *)
                    (
                        (BYTE *)notification +
                        notification
                            ->NextEntryOffset
                    );
            }
        }


        memset(
            &overlapped,
            0,
            sizeof(overlapped)
        );


        overlapped.hEvent =
            change_event;
    }


    CloseHandle(
        change_event
    );


    CloseHandle(
        directory
    );


    printf(
        "[MODULE] Watcher STOPPED\n"
    );


    return 0;
}


static int digit_modules_start_watcher(void)
{
    if (
        g_module_watcher_thread != NULL ||
        g_module_watcher_stop_event != NULL
    )
    {
        return 0;
    }


    g_module_watcher_stop_event =
        CreateEventA(
            NULL,
            TRUE,
            FALSE,
            NULL
        );


    if (
        g_module_watcher_stop_event == NULL
    )
    {
        return -1;
    }


    g_module_watcher_thread =
        CreateThread(
            NULL,
            0,
            digit_modules_watcher_main,
            NULL,
            0,
            NULL
        );


    if (
        g_module_watcher_thread == NULL
    )
    {
        CloseHandle(
            g_module_watcher_stop_event
        );


        g_module_watcher_stop_event =
            NULL;


        return -1;
    }


    return 0;
}


static void digit_modules_stop_watcher(void)
{
    if (
        g_module_watcher_stop_event != NULL
    )
    {
        SetEvent(
            g_module_watcher_stop_event
        );
    }


    if (
        g_module_watcher_thread != NULL
    )
    {
        WaitForSingleObject(
            g_module_watcher_thread,
            INFINITE
        );


        CloseHandle(
            g_module_watcher_thread
        );


        g_module_watcher_thread =
            NULL;
    }


    if (
        g_module_watcher_stop_event != NULL
    )
    {
        CloseHandle(
            g_module_watcher_stop_event
        );


        g_module_watcher_stop_event =
            NULL;
    }
}


/*
 * ------------------------------------------------
 * INITIAL DISCOVERY
 * ------------------------------------------------
 */

static int digit_modules_discover(void)
{
    char search_path[
        DIGIT_MODULE_PATH_MAX
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
        printf(
            "[MODULE] Discovery: "
            "0 directories, "
            "0 loaded, "
            "0 discovered, "
            "0 rejected\n"
        );


        return 1;
    }


    do
    {
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
                "[MODULE] REJECTED: %s "
                "(MODULE_CONF_INVALID_OR_MISSING)\n",
                find_data.cFileName
            );


            continue;
        }


        printf(
            "[MODULE] Declared ID: %s\n",
            module_id
        );


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
 * PROCESS INITIAL MODULES
 * ------------------------------------------------
 */

static int digit_modules_process(void)
{
    size_t index;


    for (
        index = 0U;
        index <
            g_module_registry.count;
        ++index
    )
    {
        stnlabz_module_record_t
            *record;

        stnlabz_module_result_t
            result;


        record =
            &g_module_registry.modules[
                index
            ];


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
                "[MODULE] Verification FAILED: "
                "%s (%s)\n",
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
                "[MODULE] Qualification FAILED: "
                "%s (%s)\n",
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
                "[MODULE] Activation FAILED: "
                "%s (%s)\n",
                record->descriptor.id,
                stnlabz_module_result_string(
                    result
                )
            );


            return 0;
        }


        (void)
        digit_modules_use_state_find_locked(
            record->descriptor.id,
            1
        );


        printf(
            "[MODULE] ACTIVE: %s\n",
            record->descriptor.id
        );
    }


    return 1;
}


/*
 * ------------------------------------------------
 * INITIALIZATION
 * ------------------------------------------------
 */

int digit_modules_init(void)
{
    char message[
        DIGIT_MODULE_AUDIT_MESSAGE_MAX
    ];

    int written;


    if (
        g_modules_initialized
    )
    {
        return -1;
    }


    InitializeCriticalSection(
        &g_module_lock
    );


    g_module_lock_initialized =
        1;


    memset(
        g_module_use,
        0,
        sizeof(g_module_use)
    );


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
        return -1;
    }


    if (
        !digit_modules_ensure_directory(
            g_modules_path
        )
    )
    {
        return -1;
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


    EnterCriticalSection(
        &g_module_lock
    );


    if (
        !digit_modules_discover() ||
        !digit_modules_process()
    )
    {
        LeaveCriticalSection(
            &g_module_lock
        );


        digit_modules_shutdown();


        return -1;
    }


    g_modules_initialized =
        1;


    LeaveCriticalSection(
        &g_module_lock
    );


    if (
        digit_modules_start_watcher() !=
        0
    )
    {
        fputs(
            "[MODULE] Watcher initialization FAILED\n",
            stderr
        );


        digit_modules_shutdown();


        return -1;
    }


    digit_modules_audit(
        "Core module subsystem initialized through "
        "STN-LABZ ABI 1.4 with live module monitoring."
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
 * SHUTDOWN
 * ------------------------------------------------
 */

void digit_modules_shutdown(void)
{
    if (
        !g_module_lock_initialized
    )
    {
        return;
    }


    digit_modules_stop_watcher();


    EnterCriticalSection(
        &g_module_lock
    );


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
            break;
        }
    }


    g_modules_initialized =
        0;


    LeaveCriticalSection(
        &g_module_lock
    );


    DeleteCriticalSection(
        &g_module_lock
    );


    g_module_lock_initialized =
        0;
}