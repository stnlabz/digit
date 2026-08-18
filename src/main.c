/**
 * @file main.c
 * @brief Entry point for the STN-LABZ Digit core.
 *
 * Digit is a private STN-LABZ engineering intelligence.
 *
 * This development entry point exercises the current machine-readable
 * policy index recognition and semantic-validation boundaries, accepts
 * a local human-operator mission, and maintains the operational runtime
 * until controlled shutdown is requested.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "digit_audit.h"
#include "digit_core.h"
#include "digit_mission.h"
#include "digit_mission_control.h"
#include "digit_policy_index.h"
#include "digit_policy_index_validate.h"
#include "platform_console.h"

 /**
  * @brief Maximum local operator command length excluding null termination.
  */
#define DIGIT_OPERATOR_COMMAND_MAX 64U

  /**
   * @brief Returns a printable semantic-validation state.
   *
   * @param state Validation state.
   *
   * @return Static state name.
   */
static const char* digit_policy_index_validation_state_name(
    digit_policy_index_validation_state_t state)
{
    switch (state)
    {
    case DIGIT_POLICY_INDEX_VALID:
        return "VALID";

    case DIGIT_POLICY_INDEX_INVALID:
        return "INVALID";

    case DIGIT_POLICY_INDEX_AMBIGUOUS:
        return "AMBIGUOUS";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Returns a printable semantic-validation reason.
 *
 * @param reason Validation reason.
 *
 * @return Static reason name.
 */
static const char* digit_policy_index_validation_reason_name(
    digit_policy_index_validation_reason_t reason)
{
    switch (reason)
    {
    case DIGIT_POLICY_INDEX_REASON_NONE:
        return "NONE";

    case DIGIT_POLICY_INDEX_REASON_ROOT_ID_INVALID:
        return "ROOT DOCUMENT ID INVALID";

    case DIGIT_POLICY_INDEX_REASON_REVISION_ID_INVALID:
        return "REVISION ID INVALID";

    case DIGIT_POLICY_INDEX_REASON_REVISION_ROOT_MISMATCH:
        return "REVISION ROOT MISMATCH";

    case DIGIT_POLICY_INDEX_REASON_PREVIOUS_REVISION_INVALID:
        return "PREVIOUS REVISION INVALID";

    case DIGIT_POLICY_INDEX_REASON_PREVIOUS_REVISION_MISMATCH:
        return "PREVIOUS REVISION MISMATCH";

    case DIGIT_POLICY_INDEX_REASON_SHA256_INVALID:
        return "SHA-256 INVALID";

    case DIGIT_POLICY_INDEX_REASON_DUPLICATE_ROOT_ID:
        return "DUPLICATE ROOT DOCUMENT ID";

    case DIGIT_POLICY_INDEX_REASON_DUPLICATE_REVISION_ID:
        return "DUPLICATE REVISION ID";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Removes trailing CR/LF characters from console input.
 *
 * @param text Mutable null-terminated string.
 *
 * @return Resulting string length.
 */
static size_t digit_console_trim_newline(
    char* text)
{
    size_t length;

    if (text == NULL)
    {
        return 0U;
    }

    length = strlen(text);

    while (length > 0U &&
        (text[length - 1U] == '\n' ||
            text[length - 1U] == '\r'))
    {
        text[length - 1U] = '\0';
        length--;
    }

    return length;
}

/**
 * @brief Runs the current local operational console.
 *
 * The runtime remains active until the local human operator explicitly
 * requests controlled shutdown.
 *
 * Mission execution capability is developed separately. This loop does
 * not manufacture behavior for a mission that Core cannot yet perform.
 *
 * @return 0 on controlled shutdown request, non-zero on console failure.
 */
static int digit_operational_runtime(void)
{
    char command[
        DIGIT_OPERATOR_COMMAND_MAX + 2U
    ];

    for (;;)
    {
        puts("");
        fputs("Digit> ", stdout);

        if (fgets(
            command,
            sizeof(command),
            stdin) == NULL)
        {
            return -1;
        }

        if (digit_console_trim_newline(
            command) == 0U)
        {
            continue;
        }

        /*
         * Current operational console command set is intentionally
         * minimal.
         */
        if (strcmp(
            command,
            "shutdown") == 0)
        {
            /*
             * Preserve explicit operator intent before leaving
             * operational runtime.
             */
            if (digit_audit_append(
                "OPERATOR",
                "Controlled shutdown requested.") != 0)
            {
                return -1;
            }

            puts(
                "Controlled shutdown requested."
            );

            return 0;
        }

        /*
         * Digit does not pretend to possess a capability that has not
         * been implemented.
         */
        puts(
            "Command not recognized."
        );
    }
}

/**
 * @brief Application entry point.
 *
 * Initializes the Windows presentation boundary and Digit Core, reads the
 * machine-readable STN-LABZ policy index, performs semantic validation of
 * controlled-document identity relationships represented by the index,
 * accepts a mission from the local human-operator interface, and maintains
 * operational runtime until controlled shutdown.
 *
 * Successful index recognition and semantic validation do not independently
 * establish authorization, cryptographic integrity, Trust Chain validity,
 * or operational acceptance.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 *
 * @return EXIT_SUCCESS on controlled completion, otherwise EXIT_FAILURE.
 */
int main(
    int argc,
    char* argv[])
{
    const char* policy_directory =
        "C:\\stn-labz\\policies";

    const char* policy_index_filename =
        "policy.index.json";

    const digit_policy_index_t* policy_index;

    digit_policy_index_validation_report_t
        validation_report;

    char mission_text[
        DIGIT_MISSION_TEXT_MAX + 2U
    ];

    size_t index;

    (void)argc;
    (void)argv;

    /*
     * Initialize platform presentation before UTF-8 content is emitted.
     */
    if (digit_platform_console_init() != 0)
    {
        fputs(
            "Platform console initialization: FAILED\n",
            stderr
        );

        return EXIT_FAILURE;
    }

    puts("STN-LABZ Digit");

    /*
     * Initialize Digit Core.
     */
    if (digit_core_init() != 0)
    {
        fputs(
            "Core initialization: FAILED\n",
            stderr
        );

        return EXIT_FAILURE;
    }

    if (digit_core_get_state() !=
        DIGIT_CORE_READY)
    {
        fputs(
            "Core initialization: FAILED\n",
            stderr
        );

        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    puts("Core initialization: READY");

    /*
     * Initialize machine-readable policy index storage.
     */
    if (digit_policy_index_init() != 0)
    {
        fputs(
            "Policy index initialization: FAILED\n",
            stderr
        );

        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    puts("");

    printf(
        "Policy index: %s\\%s\n",
        policy_directory,
        policy_index_filename
    );

    /*
     * JSON syntax and index structural recognition.
     */
    if (digit_policy_index_load(
        policy_directory,
        policy_index_filename) != 0)
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

    if (policy_index == NULL)
    {
        fputs(
            "Policy index access: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    puts(
        "Policy index recognition: PASS"
    );

    printf(
        "Policy index bytes: %zu\n",
        policy_index->bytes_read
    );

    printf(
        "Policy index records: %zu\n",
        policy_index->record_count
    );

    /*
     * Semantic validation.
     */
    if (digit_policy_index_validate(
        policy_index,
        &validation_report) != 0)
    {
        fputs(
            "Policy index semantic validation: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    puts("");

    for (index = 0U;
        index < validation_report.entry_count;
        index++)
    {
        const digit_policy_index_record_t* record;

        const digit_policy_index_validation_entry_t
            * validation;

        record =
            &policy_index->records[index];

        validation =
            &validation_report.entries[index];

        printf(
            "Root Document ID: %s\n",
            record->root_document_id
        );

        printf(
            "Revision ID: %s\n",
            record->revision_id
        );

        printf(
            "Previous Revision: %s\n",
            record->previous_revision
        );

        printf(
            "Status: %s\n",
            record->status
        );

        printf(
            "SHA-256: %s\n",
            record->sha256
        );

        printf(
            "Index semantics: %s\n",
            digit_policy_index_validation_state_name(
                validation->state
            )
        );

        printf(
            "Semantic reason: %s\n",
            digit_policy_index_validation_reason_name(
                validation->reason
            )
        );

        if (validation->state ==
            DIGIT_POLICY_INDEX_VALID)
        {
            printf(
                "Revision number: %lu\n",
                validation->revision_number
            );
        }

        puts("");
    }

    printf(
        "Valid index records: %zu\n",
        validation_report.valid_count
    );

    printf(
        "Invalid index records: %zu\n",
        validation_report.invalid_count
    );

    printf(
        "Ambiguous index records: %zu\n",
        validation_report.ambiguous_count
    );

    if (validation_report.invalid_count == 0U &&
        validation_report.ambiguous_count == 0U &&
        validation_report.valid_count ==
        policy_index->record_count)
    {
        puts(
            "Policy index semantic contract: PASS"
        );
    }
    else
    {
        puts(
            "Policy index semantic contract: PARTIAL"
        );
    }

    /*
     * Local human-operator mission assignment.
     */
    puts("");
    puts("Greetings.");
    puts("");
    puts("What is today's mission?");

    if (fgets(
        mission_text,
        sizeof(mission_text),
        stdin) == NULL)
    {
        fputs(
            "Mission input: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    if (digit_console_trim_newline(
        mission_text) == 0U)
    {
        fputs(
            "Mission assignment: REJECTED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * The current development console is a direct local human-operator
     * interface.
     */
    if (digit_mission_control_assign(
        mission_text,
        1) != 0)
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
    puts("Mission assignment: ACCEPTED");
    puts("Mission state: ACTIVE");

    /*
     * Record establishment of operational runtime before entering the
     * operator loop.
     */
    if (digit_audit_append(
        "CORE",
        "Operational runtime entered.") != 0)
    {
        fputs(
            "Operational runtime audit: FAILED\n",
            stderr
        );

        digit_policy_index_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * Enter operational runtime.
     *
     * Digit remains alive after mission acceptance.
     */
    if (digit_operational_runtime() != 0)
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
     */
    digit_policy_index_shutdown();
    digit_core_shutdown();

    return EXIT_SUCCESS;
}