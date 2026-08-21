/**
 * @file main.c
 * @brief Entry point for the STN-LABZ Digit Core.
 *
 * Digit initializes Core, establishes the internal policy-index
 * dependency required by the current policy worker, accepts a local
 * operator mission, dispatches implemented mission work, and remains
 * operational until controlled shutdown is requested.
 *
 * Policy-index processing remains an internal runtime dependency and is
 * not emitted verbosely to the operator console.
 *
 * The operational console also provides read-only access to the active
 * SQLite knowledge-store module through the Core-owned module loader.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "digit_audit.h"
#include "digit_core.h"
#include "digit_mission.h"
#include "digit_mission_control.h"
#include "digit_modules.h"
#include "digit_policy_index.h"
#include "digit_policy_worker.h"
#include "platform_console.h"


/*
 * ------------------------------------------------
 * SQLITE MODULE RETRIEVAL ABI
 * ------------------------------------------------
 *
 * Core resolves these symbols dynamically from the
 * ACTIVE sqlite module. Core does not include or link
 * against the module's private header.
 */

#define DIGIT_SQLITE_SEARCH_EXPORT \
    "digit_sqlite_search"

#define DIGIT_SQLITE_ROOT_ID_MAX \
    128

#define DIGIT_SQLITE_REVISION_ID_MAX \
    128

#define DIGIT_SQLITE_FILENAME_MAX \
    512

#define DIGIT_SQLITE_SHA256_MAX \
    65

#define DIGIT_SQLITE_STATUS_MAX \
    64

#define DIGIT_SQLITE_CHUNK_ID_MAX \
    256

#define DIGIT_SQLITE_HEADING_PATH_MAX \
    2048

#define DIGIT_SQLITE_TEXT_MAX \
    16384


typedef enum
{
    DIGIT_SQLITE_RETRIEVAL_OK = 0,

    DIGIT_SQLITE_RETRIEVAL_ERR_INVALID_ARGUMENT,

    DIGIT_SQLITE_RETRIEVAL_ERR_OPEN_FAILED,

    DIGIT_SQLITE_RETRIEVAL_ERR_QUERY_FAILED,

    DIGIT_SQLITE_RETRIEVAL_ERR_RESULT_TOO_LARGE,

    DIGIT_SQLITE_RETRIEVAL_ERR_NOT_FOUND

} digit_sqlite_retrieval_result_t;


typedef struct
{
    char chunk_id[
        DIGIT_SQLITE_CHUNK_ID_MAX
    ];

    char root_document_id[
        DIGIT_SQLITE_ROOT_ID_MAX
    ];

    char revision_id[
        DIGIT_SQLITE_REVISION_ID_MAX
    ];

    char source_filename[
        DIGIT_SQLITE_FILENAME_MAX
    ];

    char canonical_sha256[
        DIGIT_SQLITE_SHA256_MAX
    ];

    char status[
        DIGIT_SQLITE_STATUS_MAX
    ];

    unsigned int chunk_index;

    long long source_offset_start;

    long long source_offset_end;

    char heading_path[
        DIGIT_SQLITE_HEADING_PATH_MAX
    ];

    char text[
        DIGIT_SQLITE_TEXT_MAX
    ];

} digit_sqlite_search_result_t;



#define DIGIT_OPERATOR_COMMAND_MAX \
    512U

#define DIGIT_POLICY_MISSION \
    "Learn company policies"

#define DIGIT_POLICY_DIRECTORY \
    "C:\\stn-labz\\policies"

#define DIGIT_POLICY_INDEX_FILENAME \
    "policy.index.json"

#define DIGIT_KB_COMMAND_PREFIX \
    "kb "

#define DIGIT_KB_RESULT_LIMIT \
    5U


typedef digit_sqlite_retrieval_result_t
(*digit_sqlite_search_fn)(
    const char *query,
    digit_sqlite_search_result_t *results,
    size_t result_capacity,
    size_t *result_count
);


/*
 * ------------------------------------------------
 * CONSOLE INPUT
 * ------------------------------------------------
 */

static size_t digit_console_trim_newline(
    char *text
)
{
    size_t length;


    if (
        text == NULL
    )
    {
        return 0U;
    }


    length =
        strlen(
            text
        );


    while (
        length > 0U &&
        (
            text[length - 1U] == '\n' ||
            text[length - 1U] == '\r'
        )
    )
    {
        text[
            length - 1U
        ] =
            '\0';

        length--;
    }


    return length;
}


/*
 * ------------------------------------------------
 * KB RESULT STRING
 * ------------------------------------------------
 */

static const char *
digit_kb_result_string(
    digit_sqlite_retrieval_result_t result
)
{
    switch (
        result
    )
    {
        case DIGIT_SQLITE_RETRIEVAL_OK:

            return "OK";


        case DIGIT_SQLITE_RETRIEVAL_ERR_INVALID_ARGUMENT:

            return "INVALID_ARGUMENT";


        case DIGIT_SQLITE_RETRIEVAL_ERR_OPEN_FAILED:

            return "OPEN_FAILED";


        case DIGIT_SQLITE_RETRIEVAL_ERR_QUERY_FAILED:

            return "QUERY_FAILED";


        case DIGIT_SQLITE_RETRIEVAL_ERR_RESULT_TOO_LARGE:

            return "RESULT_TOO_LARGE";


        case DIGIT_SQLITE_RETRIEVAL_ERR_NOT_FOUND:

            return "NOT_FOUND";


        default:

            return "UNKNOWN";
    }
}


/*
 * ------------------------------------------------
 * KB QUERY
 * ------------------------------------------------
 */

static int digit_console_kb_query(
    const char *query
)
{
    digit_sqlite_search_fn
        search;

    digit_sqlite_search_result_t
        results[
            DIGIT_KB_RESULT_LIMIT
        ];

    digit_sqlite_retrieval_result_t
        result;

    FARPROC export_address;

    size_t result_count =
        0U;

    size_t index;

    char audit_message[
        256
    ];

    int written;


    if (
        query == NULL ||
        query[0] == '\0'
    )
    {
        puts(
            "Usage: kb <query>"
        );

        return 0;
    }


    /*
     * Resolve the retrieval function only from the
     * already-loaded, ACTIVE sqlite module.
     */

    export_address =
        digit_modules_get_export(
            "sqlite",
            DIGIT_SQLITE_SEARCH_EXPORT
        );


    if (
        export_address == NULL
    )
    {
        fputs(
            "KB retrieval unavailable: sqlite search export not available.\n",
            stderr
        );

        (void)digit_audit_append(
            "KB",
            "Knowledge query rejected: sqlite search export unavailable."
        );

        return -1;
    }


    search =
        (digit_sqlite_search_fn)
        export_address;


    memset(
        results,
        0,
        sizeof(results)
    );


    result =
        search(
            query,
            results,
            DIGIT_KB_RESULT_LIMIT,
            &result_count
        );


    if (
        result !=
        DIGIT_SQLITE_RETRIEVAL_OK
    )
    {
        fprintf(
            stderr,
            "KB query FAILED: %s\n",
            digit_kb_result_string(
                result
            )
        );


        written =
            snprintf(
                audit_message,
                sizeof(audit_message),
                "Knowledge query failed: result=%s",
                digit_kb_result_string(
                    result
                )
            );


        if (
            written >= 0 &&
            (size_t)written <
                sizeof(audit_message)
        )
        {
            (void)digit_audit_append(
                "KB",
                audit_message
            );
        }


        return -1;
    }


    written =
        snprintf(
            audit_message,
            sizeof(audit_message),
            "Knowledge query completed: results=%u",
            (unsigned int)
            result_count
        );


    if (
        written >= 0 &&
        (size_t)written <
            sizeof(audit_message)
    )
    {
        (void)digit_audit_append(
            "KB",
            audit_message
        );
    }


    if (
        result_count == 0U
    )
    {
        puts(
            "KB: no matching knowledge found."
        );

        return 0;
    }


    printf(
        "KB: %u result(s)\n",
        (unsigned int)
        result_count
    );


    for (
        index = 0U;
        index < result_count;
        ++index
    )
    {
        const digit_sqlite_search_result_t
            *entry;


        entry =
            &results[index];


        puts("");

        printf(
            "[%u] Root Document ID: %s\n",
            (unsigned int)
            (index + 1U),
            entry->root_document_id
        );


        printf(
            "    Revision ID: %s\n",
            entry->revision_id
        );


        printf(
            "    Chunk ID: %s\n",
            entry->chunk_id
        );


        printf(
            "    Chunk Index: %u\n",
            entry->chunk_index
        );


        printf(
            "    Source: %s\n",
            entry->source_filename
        );


        printf(
            "    Status: %s\n",
            entry->status
        );


        printf(
            "    Canonical SHA-256: %s\n",
            entry->canonical_sha256
        );


        printf(
            "    Source Offsets: %lld-%lld\n",
            entry->source_offset_start,
            entry->source_offset_end
        );


        printf(
            "    Heading Path: %s\n",
            entry->heading_path
        );


        printf(
            "    Text:\n%s\n",
            entry->text
        );
    }


    return 0;
}


/*
 * ------------------------------------------------
 * MISSION DISPATCH
 * ------------------------------------------------
 */

static int digit_dispatch_mission_work(
    const char *mission_text,
    const digit_policy_index_t *policy_index
)
{
    if (
        mission_text == NULL
    )
    {
        return -1;
    }


    /*
     * Only the currently established policy mission
     * activates the policy worker.
     */

    if (
        strcmp(
            mission_text,
            DIGIT_POLICY_MISSION
        ) != 0
    )
    {
        return 0;
    }


    if (
        policy_index == NULL
    )
    {
        (void)digit_audit_append(
            "POLICY_WORKER",
            "Policy worker dispatch rejected: policy index unavailable."
        );

        return -1;
    }


    if (
        digit_policy_worker_get_state() !=
        DIGIT_POLICY_WORKER_IDLE
    )
    {
        (void)digit_audit_append(
            "POLICY_WORKER",
            "Policy worker unavailable for mission dispatch."
        );

        return -1;
    }


    if (
        digit_audit_append(
            "MISSION",
            "Policy mission dispatched to policy worker."
        ) != 0
    )
    {
        return -1;
    }


    if (
        digit_policy_worker_start(
            policy_index
        ) != 0
    )
    {
        (void)digit_audit_append(
            "POLICY_WORKER",
            "Policy worker start failed."
        );

        return -1;
    }


    return 0;
}


/*
 * ------------------------------------------------
 * OPERATIONAL RUNTIME
 * ------------------------------------------------
 */

static int digit_operational_runtime(void)
{
    char command[
        DIGIT_OPERATOR_COMMAND_MAX + 2U
    ];

    size_t prefix_length;


    prefix_length =
        strlen(
            DIGIT_KB_COMMAND_PREFIX
        );


    for (;;)
    {
        puts("");

        fputs(
            "Digit> ",
            stdout
        );


        if (
            fgets(
                command,
                sizeof(command),
                stdin
            ) == NULL
        )
        {
            return -1;
        }


        if (
            digit_console_trim_newline(
                command
            ) == 0U
        )
        {
            continue;
        }


        if (
            strcmp(
                command,
                "shutdown"
            ) == 0
        )
        {
            if (
                digit_audit_append(
                    "OPERATOR",
                    "Controlled shutdown requested."
                ) != 0
            )
            {
                return -1;
            }


            puts(
                "Controlled shutdown requested."
            );


            return 0;
        }


        if (
            strncmp(
                command,
                DIGIT_KB_COMMAND_PREFIX,
                prefix_length
            ) == 0
        )
        {
            const char *query;


            query =
                command +
                prefix_length;


            if (
                digit_console_kb_query(
                    query
                ) != 0
            )
            {
                puts(
                    "KB command failed."
                );
            }


            continue;
        }


        if (
            strcmp(
                command,
                "kb"
            ) == 0
        )
        {
            puts(
                "Usage: kb <query>"
            );

            continue;
        }


        puts(
            "Command not recognized."
        );
    }
}


/*
 * ------------------------------------------------
 * APPLICATION ENTRY
 * ------------------------------------------------
 */

int main(
    int argc,
    char *argv[]
)
{
    const digit_policy_index_t *policy_index =
        NULL;

    char mission_text[
        DIGIT_MISSION_TEXT_MAX + 2U
    ];


    (void)argc;
    (void)argv;


    /*
     * Platform presentation.
     */

    if (
        digit_platform_console_init() != 0
    )
    {
        fputs(
            "Platform console initialization: FAILED\n",
            stderr
        );

        return EXIT_FAILURE;
    }


    puts(
        "STN-LABZ Digit"
    );


    /*
     * Digit Core.
     */

    if (
        digit_core_init() != 0
    )
    {
        fputs(
            "Core initialization: FAILED\n",
            stderr
        );

        return EXIT_FAILURE;
    }


    if (
        digit_core_get_state() !=
        DIGIT_CORE_READY
    )
    {
        fputs(
            "Core initialization: FAILED\n",
            stderr
        );

        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    /*
     * ------------------------------------------------
     * POLICY INDEX
     * ------------------------------------------------
     *
     * The current policy worker requires the recognized
     * policy index as immutable job input.
     *
     * This remains an internal runtime dependency.
     * The index is intentionally not dumped to the
     * operator console.
     */

    if (
        digit_policy_index_init() != 0
    )
    {
        fputs(
            "Policy index initialization: FAILED\n",
            stderr
        );

        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    if (
        digit_policy_index_load(
            DIGIT_POLICY_DIRECTORY,
            DIGIT_POLICY_INDEX_FILENAME
        ) != 0
    )
    {
        fputs(
            "Policy index recognition: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    policy_index =
        digit_policy_index_get();


    if (
        policy_index == NULL
    )
    {
        fputs(
            "Policy index access: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    (void)digit_audit_append(
        "POLICY_INDEX",
        "Policy index recognized for internal runtime use."
    );


    /*
     * Operator interface.
     */

    puts(
        "Core initialization: READY"
    );

    puts("");

    puts(
        "Greetings."
    );

    puts("");

    puts(
        "What is today's mission?"
    );


    if (
        fgets(
            mission_text,
            sizeof(mission_text),
            stdin
        ) == NULL
    )
    {
        fputs(
            "Mission input: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    if (
        digit_console_trim_newline(
            mission_text
        ) == 0U
    )
    {
        fputs(
            "Mission assignment: REJECTED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    if (
        digit_mission_control_assign(
            mission_text,
            1
        ) != 0
    )
    {
        fputs(
            "Mission assignment: REJECTED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    puts("");

    puts(
        "Mission assignment: ACCEPTED"
    );

    puts(
        "Mission state: ACTIVE"
    );


    /*
     * Mission work.
     */

    if (
        digit_dispatch_mission_work(
            mission_text,
            policy_index
        ) != 0
    )
    {
        fputs(
            "Mission work dispatch: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    /*
     * Operational runtime.
     */

    if (
        digit_audit_append(
            "CORE",
            "Operational runtime entered."
        ) != 0
    )
    {
        fputs(
            "Operational runtime audit: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    if (
        digit_operational_runtime() != 0
    )
    {
        fputs(
            "Operational runtime: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }


    /*
     * Controlled shutdown.
     *
     * Core shutdown waits for the worker before the
     * policy-index storage is withdrawn.
     */

    digit_core_shutdown();

    digit_policy_index_shutdown();


    return EXIT_SUCCESS;
}
