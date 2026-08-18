/**
 * @file digit_policy_index.h
 * @brief Machine-readable policy index interface for STN-LABZ Digit Core.
 *
 * This interface provides the initial Digit Core capability required to
 * read and structurally recognize the established STN-LABZ JSON policy
 * index.
 *
 * Successful index recognition does not establish authorization, trust,
 * authenticity, integrity, or operational acceptance.
 */

#ifndef STN_LABZ_DIGIT_POLICY_INDEX_H
#define STN_LABZ_DIGIT_POLICY_INDEX_H

#include <stddef.h>

/**
 * @brief Maximum structured-document size permitted by STN-LABZ policy.
 */
#define DIGIT_STRUCTURED_DOCUMENT_MAX 8388608U

/**
 * @brief Maximum number of policy index records retained by this stage.
 */
#define DIGIT_POLICY_INDEX_MAX_RECORDS 256U

/**
 * @brief Maximum retained root document identifier length.
 */
#define DIGIT_POLICY_INDEX_ROOT_ID_MAX 64U

/**
 * @brief Maximum retained revision identifier length.
 */
#define DIGIT_POLICY_INDEX_REVISION_ID_MAX 96U

/**
 * @brief Maximum retained title length.
 */
#define DIGIT_POLICY_INDEX_TITLE_MAX 256U

/**
 * @brief Maximum retained status length.
 */
#define DIGIT_POLICY_INDEX_STATUS_MAX 32U

/**
 * @brief Maximum retained previous-revision identifier length.
 */
#define DIGIT_POLICY_INDEX_PREVIOUS_MAX 96U

/**
 * @brief Required SHA-256 hexadecimal string length.
 */
#define DIGIT_POLICY_INDEX_SHA256_LENGTH 64U

/**
 * @brief One recognized machine-readable policy index record.
 */
typedef struct digit_policy_index_record
{
    char root_document_id[
        DIGIT_POLICY_INDEX_ROOT_ID_MAX
    ];

    char revision_id[
        DIGIT_POLICY_INDEX_REVISION_ID_MAX
    ];

    char title[
        DIGIT_POLICY_INDEX_TITLE_MAX
    ];

    char status[
        DIGIT_POLICY_INDEX_STATUS_MAX
    ];

    char previous_revision[
        DIGIT_POLICY_INDEX_PREVIOUS_MAX
    ];

    char sha256[
        DIGIT_POLICY_INDEX_SHA256_LENGTH + 1U
    ];

} digit_policy_index_record_t;

/**
 * @brief Core-owned machine-readable policy index.
 */
typedef struct digit_policy_index
{
    digit_policy_index_record_t records[
        DIGIT_POLICY_INDEX_MAX_RECORDS
    ];

    size_t record_count;
    size_t bytes_read;

} digit_policy_index_t;

/**
 * @brief Initializes Core-owned policy index state.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_policy_index_init(void);

/**
 * @brief Loads and structurally recognizes a JSON policy index.
 *
 * Current accepted contract:
 *
 * [
 *   {
 *     "root_document_id": "...",
 *     "revision_id": "...",
 *     "title": "...",
 *     "status": "...",
 *     "previous_revision": "...",
 *     "sha256": "..."
 *   }
 * ]
 *
 * All six fields are required exactly once.
 *
 * Unknown fields, duplicate fields, malformed JSON, incomplete records,
 * invalid SHA-256 syntax, excessive record count, and oversized input are
 * rejected.
 *
 * Successful recognition does not establish authorization or Trust Chain
 * validity.
 *
 * @param directory Directory containing the policy index.
 * @param filename Policy index filename.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_policy_index_load(
    const char *directory,
    const char *filename
);

/**
 * @brief Returns the active Core-owned policy index.
 *
 * @return Read-only pointer to the active index, or NULL when the index
 *         has not been initialized.
 */
const digit_policy_index_t *digit_policy_index_get(void);

/**
 * @brief Clears Core-owned policy index state.
 */
void digit_policy_index_shutdown(void);

#endif /* STN_LABZ_DIGIT_POLICY_INDEX_H */