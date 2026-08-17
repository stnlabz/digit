/**
 * @file main.c
 * @brief Entry point for the STN-LABZ Digit core.
 *
 * Digit is a private STN-LABZ engineering intelligence.
 *
 * This development entry point exercises the current Digit Core policy
 * corpus path, structural checking, and policy eligibility evaluation.
 */

#include <stdio.h>
#include <stdlib.h>

#include "digit_core.h"
#include "digit_markdown.h"
#include "digit_policy_corpus.h"
#include "digit_policy_eligibility.h"
#include "digit_policy_structure.h"
#include "platform_console.h"

/**
 * @brief Returns a printable name for a policy document state.
 *
 * @param state Policy document state.
 *
 * @return Static state name.
 */
static const char *digit_policy_state_name(
    digit_policy_document_state_t state)
{
    switch (state)
    {
        case DIGIT_POLICY_DOCUMENT_DISCOVERED:
            return "DISCOVERED";

        case DIGIT_POLICY_DOCUMENT_READ:
            return "READ";

        case DIGIT_POLICY_DOCUMENT_RECOGNIZED:
            return "RECOGNIZED";

        case DIGIT_POLICY_DOCUMENT_FAILED:
            return "FAILED";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Returns a printable name for policy structural state.
 *
 * @param state Structural state.
 *
 * @return Static state name.
 */
static const char *digit_policy_structure_state_name(
    digit_policy_structure_state_t state)
{
    switch (state)
    {
        case DIGIT_POLICY_STRUCTURE_COMPLETE:
            return "COMPLETE";

        case DIGIT_POLICY_STRUCTURE_INCOMPLETE:
            return "INCOMPLETE";

        case DIGIT_POLICY_STRUCTURE_AMBIGUOUS:
            return "AMBIGUOUS";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Returns a printable name for policy eligibility state.
 *
 * @param state Eligibility state.
 *
 * @return Static state name.
 */
static const char *digit_policy_eligibility_state_name(
    digit_policy_eligibility_state_t state)
{
    switch (state)
    {
        case DIGIT_POLICY_ELIGIBILITY_ELIGIBLE:
            return "ELIGIBLE";

        case DIGIT_POLICY_ELIGIBILITY_NOT_ELIGIBLE:
            return "NOT ELIGIBLE";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Returns a printable eligibility decision reason.
 *
 * @param reason Eligibility reason.
 *
 * @return Static reason name.
 */
static const char *digit_policy_eligibility_reason_name(
    digit_policy_eligibility_reason_t reason)
{
    switch (reason)
    {
        case DIGIT_POLICY_ELIGIBILITY_REASON_NONE:
            return "NONE";

        case DIGIT_POLICY_ELIGIBILITY_REASON_STRUCTURE_INCOMPLETE:
            return "STRUCTURE INCOMPLETE";

        case DIGIT_POLICY_ELIGIBILITY_REASON_STRUCTURE_AMBIGUOUS:
            return "STRUCTURE AMBIGUOUS";

        case DIGIT_POLICY_ELIGIBILITY_REASON_STATUS_MISSING:
            return "STATUS MISSING";

        case DIGIT_POLICY_ELIGIBILITY_REASON_STATUS_NOT_APPROVED:
            return "STATUS NOT APPROVED";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Application entry point.
 *
 * Initializes the Windows presentation boundary and Digit Core, loads the
 * local Markdown policy corpus, checks structural completeness, and evaluates
 * whether each structurally complete document is eligible to proceed to
 * later authority and provenance validation.
 *
 * Every eligibility decision includes an explicit reason.
 *
 * Eligibility does not establish policy trust, authority, authenticity,
 * mission applicability, or operational acceptance.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 *
 * @return EXIT_SUCCESS on successful corpus processing, otherwise
 *         EXIT_FAILURE.
 */
int main(
    int argc,
    char *argv[])
{
    const char *policy_directory =
        "C:\\stn-labz\\policies";

    const digit_policy_corpus_t *corpus;

    size_t index;

    size_t complete_count;
    size_t incomplete_count;
    size_t ambiguous_count;

    size_t eligible_count;
    size_t not_eligible_count;

    (void)argc;
    (void)argv;

    complete_count = 0U;
    incomplete_count = 0U;
    ambiguous_count = 0U;

    eligible_count = 0U;
    not_eligible_count = 0U;

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
     * Initialize Core-owned policy corpus storage.
     */
    if (digit_policy_corpus_init() != 0)
    {
        fputs(
            "Policy corpus initialization: FAILED\n",
            stderr
        );

        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * Load the local policy corpus.
     */
    puts("");

    printf(
        "Policy directory: %s\n",
        policy_directory
    );

    if (digit_policy_corpus_load(
            policy_directory) != 0)
    {
        fputs(
            "Policy corpus load: FAILED\n",
            stderr
        );

        digit_policy_corpus_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    corpus = digit_policy_corpus_get();

    if (corpus == NULL)
    {
        fputs(
            "Policy corpus access: FAILED\n",
            stderr
        );

        digit_policy_corpus_shutdown();
        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    puts(
        "Policy corpus enumeration: PASS"
    );

    printf(
        "Markdown documents represented: %zu\n",
        corpus->document_count
    );

    printf(
        "Markdown documents recognized: %zu\n",
        corpus->recognized_count
    );

    printf(
        "Markdown documents failed: %zu\n",
        corpus->failed_count
    );

    printf(
        "Non-Markdown files ignored: %zu\n",
        corpus->ignored_count
    );

    /*
     * Inspect every represented policy independently.
     */
    for (index = 0U;
         index < corpus->document_count;
         index++)
    {
        const digit_policy_document_t *document;

        digit_policy_structure_result_t
            structure_result;

        digit_policy_eligibility_result_t
            eligibility_result;

        const char *status;
        const char *scope;

        document =
            &corpus->documents[index];

        puts("");

        printf(
            "Policy: %s\n",
            document->filename
        );

        printf(
            "State: %s\n",
            digit_policy_state_name(
                document->state
            )
        );

        /*
         * A document that did not reach Markdown recognition cannot
         * proceed to structural or eligibility evaluation.
         */
        if (document->state !=
            DIGIT_POLICY_DOCUMENT_RECOGNIZED)
        {
            not_eligible_count++;

            puts(
                "Policy eligibility: NOT ELIGIBLE"
            );

            puts(
                "Eligibility reason: DOCUMENT NOT RECOGNIZED"
            );

            continue;
        }

        printf(
            "Bytes read: %zu\n",
            document->bytes_read
        );

        printf(
            "Lines: %zu\n",
            document->structure.total_lines
        );

        printf(
            "Headings: %zu\n",
            document->structure.heading_count
        );

        printf(
            "Metadata fields: %zu\n",
            document->structure.metadata_count
        );

        if (document->structure.heading_count > 0U)
        {
            printf(
                "First heading: %s\n",
                document->structure.first_heading
            );
        }
        else
        {
            puts(
                "First heading: NONE"
            );
        }

        status =
            digit_markdown_metadata_get(
                &document->structure,
                "Status"
            );

        scope =
            digit_markdown_metadata_get(
                &document->structure,
                "Scope"
            );

        printf(
            "Status metadata: %s\n",
            status != NULL
                ? status
                : "NOT PRESENT"
        );

        printf(
            "Scope metadata: %s\n",
            scope != NULL
                ? scope
                : "NOT PRESENT"
        );

        /*
         * Structural completeness check.
         */
        if (digit_policy_structure_check(
                &document->structure,
                &structure_result) != 0)
        {
            puts(
                "Policy structure check: FAILED"
            );

            incomplete_count++;
            not_eligible_count++;

            puts(
                "Policy eligibility: NOT ELIGIBLE"
            );

            puts(
                "Eligibility reason: STRUCTURE CHECK FAILED"
            );

            continue;
        }

        printf(
            "Policy structure: %s\n",
            digit_policy_structure_state_name(
                structure_result.state
            )
        );

        printf(
            "Status fields: %zu\n",
            structure_result.status_count
        );

        printf(
            "Scope fields: %zu\n",
            structure_result.scope_count
        );

        switch (structure_result.state)
        {
            case DIGIT_POLICY_STRUCTURE_COMPLETE:
                complete_count++;
                break;

            case DIGIT_POLICY_STRUCTURE_INCOMPLETE:
                incomplete_count++;
                break;

            case DIGIT_POLICY_STRUCTURE_AMBIGUOUS:
                ambiguous_count++;
                break;

            default:
                incomplete_count++;
                break;
        }

        /*
         * Policy eligibility evaluation.
         */
        if (digit_policy_eligibility_check(
                &document->structure,
                &structure_result,
                &eligibility_result) != 0)
        {
            puts(
                "Policy eligibility check: FAILED"
            );

            not_eligible_count++;

            continue;
        }

        printf(
            "Policy eligibility: %s\n",
            digit_policy_eligibility_state_name(
                eligibility_result.state
            )
        );

        printf(
            "Eligibility reason: %s\n",
            digit_policy_eligibility_reason_name(
                eligibility_result.reason
            )
        );

        if (eligibility_result.state ==
            DIGIT_POLICY_ELIGIBILITY_ELIGIBLE)
        {
            eligible_count++;
        }
        else
        {
            not_eligible_count++;
        }
    }

    /*
     * Corpus-level structural summary.
     */
    puts("");

    printf(
        "Structurally complete policies: %zu\n",
        complete_count
    );

    printf(
        "Structurally incomplete policies: %zu\n",
        incomplete_count
    );

    printf(
        "Structurally ambiguous policies: %zu\n",
        ambiguous_count
    );

    if (corpus->failed_count == 0U &&
        incomplete_count == 0U &&
        ambiguous_count == 0U &&
        complete_count == corpus->recognized_count)
    {
        puts(
            "Policy corpus structural contract: PASS"
        );
    }
    else
    {
        puts(
            "Policy corpus structural contract: PARTIAL"
        );
    }

    /*
     * Corpus-level eligibility summary.
     */
    puts("");

    printf(
        "Policies eligible for further validation: %zu\n",
        eligible_count
    );

    printf(
        "Policies not eligible for further validation: %zu\n",
        not_eligible_count
    );

    /*
     * Current operator interaction placeholder.
     */
    puts("");
    puts("Greetings.");
    puts("");
    puts("What is today's mission?");

    /*
     * Controlled shutdown.
     */
    digit_policy_corpus_shutdown();
    digit_core_shutdown();

    return EXIT_SUCCESS;
}