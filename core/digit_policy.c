/**
 * @file digit_policy.c
 * @brief Policy discovery and reading implementation for STN-LABZ Digit Core.
 *
 * This component identifies Markdown policy documents and provides bounded
 * document reading while delegating operating-system-specific filesystem
 * behavior to the platform layer.
 *
 * Discovery and reading do not establish policy validity, approval,
 * authority, interpretation, or operational acceptance.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "digit_policy.h"
#include "platform_fs.h"

/**
 * @brief Determines whether a filename has a Markdown .md extension.
 *
 * Extension matching is case-insensitive.
 *
 * @param filename File name to inspect.
 *
 * @return 1 when filename ends in .md, otherwise 0.
 */
static int digit_policy_is_markdown(
    const char *filename)
{
    size_t length;
    const char *extension;

    if (filename == NULL)
    {
        return 0;
    }

    length = strlen(filename);

    if (length < 3U)
    {
        return 0;
    }

    extension = filename + (length - 3U);

    if (extension[0] != '.')
    {
        return 0;
    }

    if (tolower(
            (unsigned char)extension[1]) != 'm')
    {
        return 0;
    }

    if (tolower(
            (unsigned char)extension[2]) != 'd')
    {
        return 0;
    }

    return 1;
}

/**
 * @brief Processes one filesystem entry discovered by the platform layer.
 *
 * @param filename Discovered file name.
 * @param context Pointer to digit_policy_discovery_t.
 *
 * @return 0 to continue enumeration.
 */
static int digit_policy_discovery_callback(
    const char *filename,
    void *context)
{
    digit_policy_discovery_t *result;

    if (filename == NULL ||
        context == NULL)
    {
        return 0;
    }

    result = (digit_policy_discovery_t *)context;

    if (digit_policy_is_markdown(
            filename) != 0)
    {
        result->markdown_files++;

        printf(
            "Policy discovered: %s\n",
            filename
        );
    }
    else
    {
        result->ignored_files++;
    }

    return 0;
}

int digit_policy_discover(
    const char *directory,
    digit_policy_discovery_t *result)
{
    if (directory == NULL ||
        result == NULL)
    {
        return -1;
    }

    result->markdown_files = 0U;
    result->ignored_files = 0U;

    if (digit_platform_enumerate_files(
            directory,
            digit_policy_discovery_callback,
            result) != 0)
    {
        return -1;
    }

    return 0;
}

int digit_policy_read(
    const char *directory,
    const char *filename,
    char *buffer,
    size_t capacity,
    size_t *bytes_read)
{
    if (directory == NULL ||
        filename == NULL ||
        buffer == NULL ||
        bytes_read == NULL)
    {
        return -1;
    }

    /*
     * Digit Policy accepts Markdown documents only.
     */
    if (digit_policy_is_markdown(
            filename) == 0)
    {
        return -1;
    }

    if (capacity > (DIGIT_POLICY_DOCUMENT_MAX + 1U))
    {
        capacity = DIGIT_POLICY_DOCUMENT_MAX + 1U;
    }

    return digit_platform_read_file(
        directory,
        filename,
        buffer,
        capacity,
        bytes_read
    );
}