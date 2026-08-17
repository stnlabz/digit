/**
 * @file main.c
 * @brief Entry point for the STN-LABZ Digit core.
 *
 * Digit is a private STN-LABZ engineering intelligence.
 * This file provides the process entry point and delegates Core lifecycle
 * responsibility to Digit Core.
 */

#include <stdio.h>
#include <stdlib.h>

#include "digit_core.h"
#include "digit_policy.h"

/**
 * @brief Application entry point.
 *
 * Initializes Digit Core, performs development-stage policy discovery,
 * and reads one known Markdown policy document into bounded memory.
 *
 * Policy interpretation and validation are intentionally not performed
 * during this development step.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 *
 * @return EXIT_SUCCESS on successful operation, otherwise EXIT_FAILURE.
 */
int main(
    int argc,
    char *argv[])
{
    const char *policy_directory =
        "C:\\stn-labz\\policies";

    const char *test_policy =
        "20260801.1_KISS_THEORY.md";

    digit_policy_discovery_t discovery;

    char policy_buffer[
        DIGIT_POLICY_DOCUMENT_MAX + 1U
    ];

    size_t policy_bytes;

    (void)argc;
    (void)argv;

    puts("STN-LABZ Digit");

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
     * Development-stage policy discovery.
     */
    puts("");

    printf(
        "Policy directory: %s\n",
        policy_directory
    );

    if (digit_policy_discover(
            policy_directory,
            &discovery) != 0)
    {
        fputs(
            "Policy discovery: FAILED\n",
            stderr
        );

        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    printf(
        "Markdown policies discovered: %zu\n",
        discovery.markdown_files
    );

    printf(
        "Non-Markdown files ignored: %zu\n",
        discovery.ignored_files
    );

    /*
     * Development-stage policy read.
     *
     * The raw document body is intentionally not presented to the
     * terminal. Presentation of document content will have its own
     * validation and safety requirements.
     */
    puts("");

    printf(
        "Policy read target: %s\n",
        test_policy
    );

    if (digit_policy_read(
            policy_directory,
            test_policy,
            policy_buffer,
            sizeof(policy_buffer),
            &policy_bytes) != 0)
    {
        fputs(
            "Policy read: FAILED\n",
            stderr
        );

        digit_core_shutdown();

        return EXIT_FAILURE;
    }

    printf(
        "Policy read: PASS\n"
    );

    printf(
        "Policy bytes read: %zu\n",
        policy_bytes
    );

    puts("");
    puts("Greetings.");
    puts("");
    puts("What is today's mission?");

    digit_core_shutdown();

    return EXIT_SUCCESS;
}