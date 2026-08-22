/**
 * @file main.c
 * @brief Entry point for the STN-LABZ Digit Core.
 *
 * Digit initializes Core, accepts a local operator mission,
 * provides access to the persistent knowledge base through
 * qualified modules, and remains operational until controlled
 * shutdown is requested.
 *
 * Digit does not independently traverse, parse, validate, or
 * ingest the controlled-document repository.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "module.h"

#include "digit_audit.h"
#include "digit_core.h"
#include "digit_mission.h"
#include "digit_mission_control.h"
#include "digit_modules.h"
#include "platform_console.h"


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


typedef digit_sqlite_retrieval_result_t
(*digit_sqlite_search_fn)(
    const char *query,
    digit_sqlite_search_result_t *results,
    size_t result_capacity,
    size_t *result_count
);


#define DIGIT_OPERATOR_COMMAND_MAX \
    512U

#define DIGIT_KB_COMMAND_PREFIX \
    "kb "

#define DIGIT_KB_RESULT_LIMIT \
    5U


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


    return
        length;
}


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


static void digit_console_emit_directives(
    const char *text
)
{
    const char *cursor;


    if (
        text == NULL
    )
    {
        return;
    }


    cursor =
        text;


    while (
        *cursor != '\0'
    )
    {
        const char *line_end;

        size_t line_length;


        line_end =
            strchr(
                cursor,
                '\n'
            );


        if (
            line_end != NULL
        )
        {
            line_length =
                (size_t)(
                    line_end -
                    cursor
                );
        }
        else
        {
            line_length =
                strlen(
                    cursor
                );
        }


        if (
            line_length > 0U &&
            cursor[
                line_length - 1U
            ] == '\r'
        )
        {
            line_length--;
        }


        if (
            line_length >= 4U &&
            cursor[0] == '*' &&
            cursor[1] == '*' &&
            cursor[
                line_length - 2U
            ] == '*' &&
            cursor[
                line_length - 1U
            ] == '*'
        )
        {
            fwrite(
                cursor + 2,
                1U,
                line_length - 4U,
                stdout
            );


            fputc(
                '\n',
                stdout
            );
        }


        if (
            line_end == NULL
        )
        {
            break;
        }


        cursor =
            line_end + 1;
    }
}


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

    FARPROC export_address =
        NULL;

    size_t result_count =
        0U;

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


    if (
        digit_modules_acquire_export(
            "sqlite",
            DIGIT_SQLITE_SEARCH_EXPORT,
            &export_address
        ) != 0 ||
        export_address == NULL
    )
    {
        fputs(
            "KB retrieval temporarily unavailable.\n",
            stderr
        );


        (void)digit_audit_append(
            "KB",
            "Knowledge query rejected: "
            "sqlite module unavailable or replacing."
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


    /*
     * Release the module lease immediately after
     * returning from module code.
     */

    digit_modules_release_export(
        "sqlite"
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
            "UNKNOWN"
        );


        return 0;
    }


    digit_console_emit_directives(
        results[0].text
    );


    return 0;
}


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


int main(
    int argc,
    char *argv[]
)
{
    char mission_text[
        DIGIT_MISSION_TEXT_MAX + 2U
    ];


    (void)argc;
    (void)argv;


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


    printf(
        "Digit is using the STN-LABZ ABI version %u.%u\n",
        STNLABZ_MODULE_API_MAJOR,
        STNLABZ_MODULE_API_MINOR
    );


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


        digit_core_shutdown();


        return EXIT_FAILURE;
    }


    digit_core_shutdown();


    return EXIT_SUCCESS;
}