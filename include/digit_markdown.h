/**
 * @file digit_markdown.h
 * @brief Initial Markdown structural recognition for STN-LABZ Digit Core.
 *
 * Markdown understanding is a core Digit responsibility.
 *
 * This interface represents the earliest implementation stage only:
 * structural recognition of ATX headings in a Markdown document.
 *
 * Full GitHub Flavored Markdown support will be developed and qualified
 * incrementally.
 */

#ifndef STN_LABZ_DIGIT_MARKDOWN_H
#define STN_LABZ_DIGIT_MARKDOWN_H

#include <stddef.h>

/**
 * @brief Maximum heading text retained during structural recognition.
 */
#define DIGIT_MARKDOWN_HEADING_MAX 256U

/**
 * @brief Structural information recognized from a Markdown document.
 */
typedef struct digit_markdown_structure
{
    size_t total_lines;
    size_t heading_count;

    unsigned int first_heading_level;

    char first_heading[
        DIGIT_MARKDOWN_HEADING_MAX
    ];
} digit_markdown_structure_t;

/**
 * @brief Performs initial structural recognition of Markdown content.
 *
 * This function currently recognizes ATX headings only.
 *
 * Recognition does not establish:
 *
 * - document validity;
 * - policy approval;
 * - policy authority;
 * - semantic meaning;
 * - operational acceptance.
 *
 * @param markdown Null-terminated Markdown document.
 * @param structure Receives recognized structural information.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_markdown_recognize(
    const char *markdown,
    digit_markdown_structure_t *structure
);

#endif /* STN_LABZ_DIGIT_MARKDOWN_H */