/**
 * @file digit_markdown.c
 * @brief Initial Markdown structural recognition for STN-LABZ Digit Core.
 *
 * This component implements the earliest deterministic Markdown recognition
 * required by Digit Core.
 *
 * Current scope:
 *
 * - count document lines;
 * - recognize ATX headings;
 * - count recognized headings;
 * - preserve the first heading level;
 * - preserve the first heading text.
 *
 * This is not a complete GitHub Flavored Markdown parser.
 */

#include <stddef.h>
#include <string.h>

#include "digit_markdown.h"

/**
 * @brief Copies bounded heading text into the structural result.
 *
 * Leading heading markers and the required separating space are not copied.
 *
 * Trailing carriage-return and newline characters are excluded.
 *
 * @param destination Destination buffer.
 * @param capacity Destination capacity.
 * @param start Beginning of heading text.
 * @param length Heading text length.
 */
static void digit_markdown_copy_heading(
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
 * characters followed by either a space or the end of the line.
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
     * A heading marker must be followed by whitespace or line end.
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
     * Remove optional closing '#' sequence when separated from
     * heading text by whitespace.
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
         * Strip a CR when reading CRLF input.
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

                digit_markdown_copy_heading(
                    structure->first_heading,
                    sizeof(
                        structure->first_heading
                    ),
                    heading_text,
                    heading_text_length
                );
            }
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