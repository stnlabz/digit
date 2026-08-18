/**
 * @file digit_policy_index_validate.h
 * @brief Semantic validation for the STN-LABZ machine policy index.
 *
 * This interface validates relationships between machine-readable policy
 * index fields after JSON syntax and structural recognition have completed.
 *
 * Semantic validation does not establish authorization, cryptographic
 * integrity, Trust Chain PASS, or operational acceptance.
 */

#ifndef STN_LABZ_DIGIT_POLICY_INDEX_VALIDATE_H
#define STN_LABZ_DIGIT_POLICY_INDEX_VALIDATE_H

#include <stddef.h>

#include "digit_policy_index.h"

/**
 * @brief Semantic validation state for one index record.
 */
typedef enum digit_policy_index_validation_state
{
    DIGIT_POLICY_INDEX_VALID = 0,
    DIGIT_POLICY_INDEX_INVALID,
    DIGIT_POLICY_INDEX_AMBIGUOUS
} digit_policy_index_validation_state_t;

/**
 * @brief Deterministic semantic-validation reason.
 */
typedef enum digit_policy_index_validation_reason
{
    DIGIT_POLICY_INDEX_REASON_NONE = 0,

    DIGIT_POLICY_INDEX_REASON_ROOT_ID_INVALID,
    DIGIT_POLICY_INDEX_REASON_REVISION_ID_INVALID,
    DIGIT_POLICY_INDEX_REASON_REVISION_ROOT_MISMATCH,

    DIGIT_POLICY_INDEX_REASON_PREVIOUS_REVISION_INVALID,
    DIGIT_POLICY_INDEX_REASON_PREVIOUS_REVISION_MISMATCH,

    DIGIT_POLICY_INDEX_REASON_SHA256_INVALID,

    DIGIT_POLICY_INDEX_REASON_DUPLICATE_ROOT_ID,
    DIGIT_POLICY_INDEX_REASON_DUPLICATE_REVISION_ID

} digit_policy_index_validation_reason_t;

/**
 * @brief Validation result for one machine index record.
 */
typedef struct digit_policy_index_validation_entry
{
    digit_policy_index_validation_state_t state;
    digit_policy_index_validation_reason_t reason;

    unsigned long revision_number;

} digit_policy_index_validation_entry_t;

/**
 * @brief Corpus-level semantic validation report.
 */
typedef struct digit_policy_index_validation_report
{
    digit_policy_index_validation_entry_t entries[
        DIGIT_POLICY_INDEX_MAX_RECORDS
    ];

    size_t entry_count;

    size_t valid_count;
    size_t invalid_count;
    size_t ambiguous_count;

} digit_policy_index_validation_report_t;

/**
 * @brief Performs semantic validation of a recognized policy index.
 *
 * Current rules:
 *
 * - root_document_id must be non-empty;
 * - revision_id must equal:
 *
 *       <root_document_id>.R<number>
 *
 * - R0 requires previous_revision == "NONE";
 * - Rn, where n > 0, requires:
 *
 *       previous_revision == <root_document_id>.R(n - 1)
 *
 * - sha256 must contain exactly 64 hexadecimal characters;
 * - duplicate root_document_id values are ambiguous;
 * - duplicate revision_id values are ambiguous.
 *
 * Validation of a record does not establish that Status is authorized.
 *
 * @param index Recognized machine policy index.
 * @param report Receives semantic-validation results.
 *
 * @return 0 when validation completes, non-zero on invalid arguments.
 */
int digit_policy_index_validate(
    const digit_policy_index_t *index,
    digit_policy_index_validation_report_t *report
);

#endif /* STN_LABZ_DIGIT_POLICY_INDEX_VALIDATE_H */