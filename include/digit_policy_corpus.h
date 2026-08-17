/**
 * @file digit_policy_corpus.h
 * @brief Policy corpus loading interface for STN-LABZ Digit Core.
 *
 * The policy corpus loader discovers Markdown policy documents,
 * reads each document into bounded memory, and performs Markdown
 * structural recognition independently for each document.
 *
 * Corpus storage is owned by Digit Core.
 *
 * Corpus loading does not establish policy validity, approval,
 * authority, trust, or operational acceptance.
 */

#ifndef STN_LABZ_DIGIT_POLICY_CORPUS_H
#define STN_LABZ_DIGIT_POLICY_CORPUS_H

#include <stddef.h>

#include "digit_markdown.h"

/**
 * @brief Maximum number of policy documents retained in one corpus.
 */
#define DIGIT_POLICY_CORPUS_MAX 128U

/**
 * @brief Maximum retained policy filename length.
 */
#define DIGIT_POLICY_FILENAME_MAX 260U

/**
 * @brief Result state for one policy document.
 */
typedef enum digit_policy_document_state
{
    DIGIT_POLICY_DOCUMENT_DISCOVERED = 0,
    DIGIT_POLICY_DOCUMENT_READ,
    DIGIT_POLICY_DOCUMENT_RECOGNIZED,
    DIGIT_POLICY_DOCUMENT_FAILED
} digit_policy_document_state_t;

/**
 * @brief One policy document represented in the loaded corpus.
 *
 * Raw document content is not retained after structural recognition
 * during this development stage.
 */
typedef struct digit_policy_document
{
    char filename[DIGIT_POLICY_FILENAME_MAX];

    digit_policy_document_state_t state;

    size_t bytes_read;

    digit_markdown_structure_t structure;

} digit_policy_document_t;

/**
 * @brief Loaded policy corpus.
 */
typedef struct digit_policy_corpus
{
    digit_policy_document_t documents[
        DIGIT_POLICY_CORPUS_MAX
    ];

    size_t document_count;
    size_t recognized_count;
    size_t failed_count;
    size_t ignored_count;

} digit_policy_corpus_t;

/**
 * @brief Initializes Core-owned policy corpus storage.
 *
 * Existing corpus state is cleared.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_policy_corpus_init(void);

/**
 * @brief Loads and structurally recognizes the local Markdown policy corpus.
 *
 * Each Markdown document is processed independently.
 *
 * Failure of one document is preserved as that document's result and does
 * not convert successfully processed documents into failures.
 *
 * The operation fails at the corpus level only when the directory cannot
 * be enumerated or the supplied directory is invalid.
 *
 * @param directory Local policy directory.
 *
 * @return 0 when corpus enumeration completes, non-zero on corpus-level
 *         failure.
 */
int digit_policy_corpus_load(
    const char *directory
);

/**
 * @brief Returns the Core-owned policy corpus.
 *
 * The returned pointer provides read-only access to corpus state.
 *
 * @return Pointer to the active policy corpus.
 */
const digit_policy_corpus_t *digit_policy_corpus_get(void);

/**
 * @brief Clears Core-owned policy corpus storage.
 */
void digit_policy_corpus_shutdown(void);

#endif /* STN_LABZ_DIGIT_POLICY_CORPUS_H */