/**
 * @file digit_markdown.c
 * @brief Markdown structural recognition for STN-LABZ Digit Core.
 *
 * Current scope:
 *
 * - count document lines;
 * - recognize ATX headings;
 * - preserve the first heading;
 * - recognize simple Markdown metadata;
 * - recognize STN-LABZ bold metadata using "**Key:** Value".
 *
 * This is not a complete GitHub Flavored Markdown parser.
 */

#include <stddef.h>
#include <string.h>

#include "digit_markdown.h"

/**
 * @brief Copies bounded text into a destination buffer.
 *
 * @param destination Destination buffer.
 * @param capacity Destination buffer capacity.
 * @param start Beginning of source text.
 * @param length Number of source bytes to copy.
 */
static void digit_markdown_copy_text(
    char *destination,
    size_t capacity,
    const char *start,
    size_t length)
{
    size_t copy_length;

    if (destination == NULL ||
        start == NULL ||
        capacity == 0U)
    {
        return;
    }

    copy_length = length;

    if (copy_length >= capacity)
    {
        copy_length = capacity - 1U;
    }

    if (copy_length > 0U)
    {
        memcpy(
            destination,
            start,
            copy_length
        );
    }

    destination[copy_length] = '\0';
}

/**
 * @brief Examines one Markdown line for an ATX heading.
 *
 * Valid headings recognized during this stage use one through six '#'
 * characters followed by whitespace or the end of the line.
 *
 * @param line Beginning of line.
 * @param length Length of line excluding newline characters.
 * @param level Receives heading level when recognized.
 * @param text_start Receives beginning of heading text.
 * @param text_length Receives heading text length.
 *
 * @return 1 when a heading is recognized, otherwise 0.
 */
static int digit_markdown_parse_atx_heading(
    const char *line,
    size_t length,
    unsigned int *level,
    const char **text_start,
    size_t *text_length)
{
    size_t index;
    size_t heading_start;
    size_t heading_length;

    if (line == NULL ||
        level == NULL ||
        text_start == NULL ||
        text_length == NULL ||
        length == 0U)
    {
        return 0;
    }

    index = 0U;

    while (index < length &&
           line[index] == '#' &&
           index < 6U)
    {
        index++;
    }

    if (index == 0U)
    {
        return 0;
    }

    /*
     * More than six consecutive '#' characters do not form
     * an ATX heading.
     */
    if (index < length &&
        line[index] == '#')
    {
        return 0;
    }

    /*
     * Heading marker must be followed by whitespace or line end.
     */
    if (index < length &&
        line[index] != ' ' &&
        line[index] != '\t')
    {
        return 0;
    }

    *level = (unsigned int)index;

    while (index < length &&
           (line[index] == ' ' ||
            line[index] == '\t'))
    {
        index++;
    }

    heading_start = index;
    heading_length = length - heading_start;

    /*
     * Remove trailing spaces and tabs.
     */
    while (heading_length > 0U &&
           (line[
                heading_start +
                heading_length -
                1U
            ] == ' ' ||
            line[
                heading_start +
                heading_length -
                1U
            ] == '\t'))
    {
        heading_length--;
    }

    /*
     * Remove an optional closing ATX '#' sequence.
     */
    if (heading_length > 0U)
    {
        size_t end;
        size_t hashes;

        end = heading_start + heading_length;
        hashes = end;

        while (hashes > heading_start &&
               line[hashes - 1U] == '#')
        {
            hashes--;
        }

        if (hashes < end &&
            hashes > heading_start &&
            (line[hashes - 1U] == ' ' ||
             line[hashes - 1U] == '\t'))
        {
            heading_length =
                hashes -
                heading_start -
                1U;

            while (heading_length > 0U &&
                   (line[
                        heading_start +
                        heading_length -
                        1U
                    ] == ' ' ||
                    line[
                        heading_start +
                        heading_length -
                        1U
                    ] == '\t'))
            {
                heading_length--;
            }
        }
    }

    *text_start = line + heading_start;
    *text_length = heading_length;

    return 1;
}

/**
 * @brief Parses STN-LABZ bold Markdown metadata.
 *
 * Expected form:
 *
 *     **Key:** Value
 *
 * Example:
 *
 *     **Status:** Approved
 *
 * @param line Beginning of line.
 * @param length Length of line.
 * @param key_start Receives beginning of metadata key.
 * @param key_length Receives metadata key length.
 * @param value_start Receives beginning of metadata value.
 * @param value_length Receives metadata value length.
 *
 * @return 1 when metadata is recognized, otherwise 0.
 */
static int digit_markdown_parse_bold_metadata(
    const char *line,
    size_t length,
    const char **key_start,
    size_t *key_length,
    const char **value_start,
    size_t *value_length)
{
    size_t colon;
    size_t value_begin;
    size_t value_end;

    if (line == NULL ||
        key_start == NULL ||
        key_length == NULL ||
        value_start == NULL ||
        value_length == NULL ||
        length < 7U)
    {
        return 0;
    }

    /*
     * STN-LABZ metadata begins with Markdown bold markers.
     */
    if (line[0] != '*' ||
        line[1] != '*')
    {
        return 0;
    }

    colon = 2U;

    while (colon < length &&
           line[colon] != ':')
    {
        colon++;
    }

    /*
     * A key must exist between the opening "**" and ':'.
     */
    if (colon <= 2U ||
        colon >= length)
    {
        return 0;
    }

    /*
     * The colon must be immediately followed by the closing "**".
     *
     * Expected:
     *
     * **Status:** Approved
     */
    if ((colon + 2U) >= length ||
        line[colon + 1U] != '*' ||
        line[colon + 2U] != '*')
    {
        return 0;
    }

    value_begin = colon + 3U;

    while (value_begin < length &&
           (line[value_begin] == ' ' ||
            line[value_begin] == '\t'))
    {
        value_begin++;
    }

    value_end = length;

    while (value_end > value_begin &&
           (line[value_end - 1U] == ' ' ||
            line[value_end - 1U] == '\t'))
    {
        value_end--;
    }

    /*
     * Empty values are not retained during this recognition stage.
     */
    if (value_begin >= value_end)
    {
        return 0;
    }

    *key_start = line + 2U;
    *key_length = colon - 2U;

    *value_start = line + value_begin;
    *value_length = value_end - value_begin;

    return 1;
}

/**
 * @brief Parses plain metadata using "Key: Value" syntax.
 *
 * This form remains supported for structured Markdown documents that do
 * not use bold metadata labels.
 *
 * @param line Beginning of line.
 * @param length Length of line.
 * @param key_start Receives beginning of metadata key.
 * @param key_length Receives metadata key length.
 * @param value_start Receives beginning of metadata value.
 * @param value_length Receives metadata value length.
 *
 * @return 1 when metadata is recognized, otherwise 0.
 */
static int digit_markdown_parse_plain_metadata(
    const char *line,
    size_t length,
    const char **key_start,
    size_t *key_length,
    const char **value_start,
    size_t *value_length)
{
    size_t colon;
    size_t key_end;
    size_t value_begin;
    size_t value_end;

    if (line == NULL ||
        key_start == NULL ||
        key_length == NULL ||
        value_start == NULL ||
        value_length == NULL ||
        length == 0U)
    {
        return 0;
    }

    /*
     * Structural Markdown lines are not treated as plain metadata.
     */
    if (line[0] == '#' ||
        line[0] == '>' ||
        line[0] == '-' ||
        line[0] == '*' ||
        line[0] == '`')
    {
        return 0;
    }

    colon = 0U;

    while (colon < length &&
           line[colon] != ':')
    {
        colon++;
    }

    if (colon == 0U ||
        colon >= length)
    {
        return 0;
    }

    key_end = colon;

    while (key_end > 0U &&
           (line[key_end - 1U] == ' ' ||
            line[key_end - 1U] == '\t'))
    {
        key_end--;
    }

    if (key_end == 0U)
    {
        return 0;
    }

    value_begin = colon + 1U;

    while (value_begin < length &&
           (line[value_begin] == ' ' ||
            line[value_begin] == '\t'))
    {
        value_begin++;
    }

    value_end = length;

    while (value_end > value_begin &&
           (line[value_end - 1U] == ' ' ||
            line[value_end - 1U] == '\t'))
    {
        value_end--;
    }

    if (value_begin >= value_end)
    {
        return 0;
    }

    *key_start = line;
    *key_length = key_end;

    *value_start = line + value_begin;
    *value_length = value_end - value_begin;

    return 1;
}

/**
 * @brief Attempts to recognize supported metadata syntax.
 *
 * STN-LABZ bold metadata is attempted first, followed by the plain
 * "Key: Value" form.
 *
 * @param line Beginning of line.
 * @param length Length of line.
 * @param key_start Receives beginning of metadata key.
 * @param key_length Receives metadata key length.
 * @param value_start Receives beginning of metadata value.
 * @param value_length Receives metadata value length.
 *
 * @return 1 when metadata is recognized, otherwise 0.
 */
static int digit_markdown_parse_metadata(
    const char *line,
    size_t length,
    const char **key_start,
    size_t *key_length,
    const char **value_start,
    size_t *value_length)
{
    if (digit_markdown_parse_bold_metadata(
            line,
            length,
            key_start,
            key_length,
            value_start,
            value_length) != 0)
    {
        return 1;
    }

    return digit_markdown_parse_plain_metadata(
        line,
        length,
        key_start,
        key_length,
        value_start,
        value_length
    );
}

int digit_markdown_recognize(
    const char *markdown,
    digit_markdown_structure_t *structure)
{
    const char *cursor;

    if (markdown == NULL ||
        structure == NULL)
    {
        return -1;
    }

    memset(
        structure,
        0,
        sizeof(*structure)
    );

    cursor = markdown;

    while (*cursor != '\0')
    {
        const char *line_start;
        const char *line_end;
        size_t line_length;

        unsigned int heading_level;
        const char *heading_text;
        size_t heading_text_length;

        const char *metadata_key;
        size_t metadata_key_length;

        const char *metadata_value;
        size_t metadata_value_length;

        line_start = cursor;
        line_end = cursor;

        while (*line_end != '\0' &&
               *line_end != '\n')
        {
            line_end++;
        }

        line_length =
            (size_t)(line_end - line_start);

        /*
         * Strip CR from CRLF input.
         */
        if (line_length > 0U &&
            line_start[line_length - 1U] == '\r')
        {
            line_length--;
        }

        structure->total_lines++;

        if (digit_markdown_parse_atx_heading(
                line_start,
                line_length,
                &heading_level,
                &heading_text,
                &heading_text_length) != 0)
        {
            structure->heading_count++;

            if (structure->heading_count == 1U)
            {
                structure->first_heading_level =
                    heading_level;

                digit_markdown_copy_text(
                    structure->first_heading,
                    sizeof(
                        structure->first_heading
                    ),
                    heading_text,
                    heading_text_length
                );
            }
        }
        else if (
            structure->metadata_count <
                DIGIT_MARKDOWN_METADATA_MAX &&
            digit_markdown_parse_metadata(
                line_start,
                line_length,
                &metadata_key,
                &metadata_key_length,
                &metadata_value,
                &metadata_value_length) != 0)
        {
            digit_markdown_metadata_t *field;

            field =
                &structure->metadata[
                    structure->metadata_count
                ];

            digit_markdown_copy_text(
                field->key,
                sizeof(field->key),
                metadata_key,
                metadata_key_length
            );

            digit_markdown_copy_text(
                field->value,
                sizeof(field->value),
                metadata_value,
                metadata_value_length
            );

            structure->metadata_count++;
        }

        if (*line_end == '\n')
        {
            cursor = line_end + 1;
        }
        else
        {
            cursor = line_end;
        }
    }

    return 0;
}

const char *digit_markdown_metadata_get(
    const digit_markdown_structure_t *structure,
    const char *key)
{
    size_t index;

    if (structure == NULL ||
        key == NULL)
    {
        return NULL;
    }

    for (index = 0U;
         index < structure->metadata_count;
         index++)
    {
        if (strcmp(
                structure->metadata[index].key,
                key) == 0)
        {
            return structure->metadata[index].value;
        }
    }

    return NULL;
}