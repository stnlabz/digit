/**
 * @file digit_markdown.h
 * @brief Markdown structural recognition for STN-LABZ Digit Core.
 *
 * Markdown understanding is a core Digit responsibility.
 *
 * Current implementation provides:
 *
 * - ATX heading recognition;
 * - document line counting;
 * - simple metadata recognition using "Key: Value" syntax.
 *
 * Full GitHub Flavored Markdown support will be developed and qualified
 * incrementally.
 */

#ifndef STN_LABZ_DIGIT_MARKDOWN_H
#define STN_LABZ_DIGIT_MARKDOWN_H

#include <stddef.h>

#define DIGIT_MARKDOWN_HEADING_MAX 256U
#define DIGIT_MARKDOWN_METADATA_KEY_MAX 64U
#define DIGIT_MARKDOWN_METADATA_VALUE_MAX 256U
#define DIGIT_MARKDOWN_METADATA_MAX 32U

/**
 * @brief One recognized Markdown metadata field.
 */
typedef struct digit_markdown_metadata
{
    char key[DIGIT_MARKDOWN_METADATA_KEY_MAX];
    char value[DIGIT_MARKDOWN_METADATA_VALUE_MAX];
} digit_markdown_metadata_t;

/**
 * @brief Structural information recognized from a Markdown document.
 */
typedef struct digit_markdown_structure
{
    size_t total_lines;
    size_t heading_count;
    size_t metadata_count;

    unsigned int first_heading_level;

    char first_heading[
        DIGIT_MARKDOWN_HEADING_MAX
    ];

    digit_markdown_metadata_t metadata[
        DIGIT_MARKDOWN_METADATA_MAX
    ];

} digit_markdown_structure_t;

/**
 * @brief Performs structural recognition of Markdown content.
 *
 * Recognition currently includes ATX headings and simple "Key: Value"
 * metadata lines.
 *
 * Recognition does not establish:
 *
 * - document validity;
 * - policy approval;
 * - policy authority;
 * - semantic correctness;
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

/**
 * @brief Finds a recognized metadata field by exact key.
 *
 * @param structure Recognized Markdown structure.
 * @param key Metadata key to locate.
 *
 * @return Pointer to the recognized metadata value, or NULL when absent.
 */
const char *digit_markdown_metadata_get(
    const digit_markdown_structure_t *structure,
    const char *key
);

#endif /* STN_LABZ_DIGIT_MARKDOWN_H */