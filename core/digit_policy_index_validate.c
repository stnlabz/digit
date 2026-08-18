/**
 * @file digit_policy_index_validate.c
 * @brief Semantic validation for the STN-LABZ machine policy index.
 *
 * This component validates controlled-document identity relationships
 * represented by the machine-readable policy index.
 *
 * JSON parsing and semantic validation remain separate boundaries.
 *
 * A semantically valid index record is not automatically authorized,
 * cryptographically valid, trusted, or accepted.
 */

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "digit_policy_index_validate.h"

/**
 * @brief Validates SHA-256 hexadecimal representation.
 *
 * @param sha256 SHA-256 string.
 *
 * @return 1 when valid, otherwise 0.
 */
static int digit_policy_index_validate_sha256(
    const char *sha256)
{
    size_t index;

    if (sha256 == NULL)
    {
        return 0;
    }

    if (strlen(sha256) !=
        DIGIT_POLICY_INDEX_SHA256_LENGTH)
    {
        return 0;
    }

    for (index = 0U;
         index < DIGIT_POLICY_INDEX_SHA256_LENGTH;
         index++)
    {
        if (isxdigit(
                (unsigned char)sha256[index]) == 0)
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Extracts and validates the revision number.
 *
 * Expected revision format:
 *
 *     <root_document_id>.R<number>
 *
 * Examples:
 *
 *     GH-001.R0
 *     20260817.1.R0
 *     20260730.1.R2
 *
 * @param root_document_id Root document identity.
 * @param revision_id Revision identity.
 * @param revision_number Receives parsed revision number.
 *
 * @return 0 on success, non-zero on invalid format or root mismatch.
 */
static int digit_policy_index_parse_revision(
    const char *root_document_id,
    const char *revision_id,
    unsigned long *revision_number)
{
    size_t root_length;
    const char *number_start;
    const char *cursor;
    char *end_pointer;
    unsigned long number;

    if (root_document_id == NULL ||
        revision_id == NULL ||
        revision_number == NULL)
    {
        return -1;
    }

    if (root_document_id[0] == '\0' ||
        revision_id[0] == '\0')
    {
        return -1;
    }

    root_length =
        strlen(root_document_id);

    /*
     * Revision must begin with the complete permanent root identity.
     */
    if (strncmp(
            revision_id,
            root_document_id,
            root_length) != 0)
    {
        return -2;
    }

    /*
     * Root identity must be followed immediately by ".R".
     */
    if (revision_id[root_length] != '.' ||
        revision_id[root_length + 1U] != 'R')
    {
        return -1;
    }

    number_start =
        revision_id +
        root_length +
        2U;

    if (*number_start == '\0')
    {
        return -1;
    }

    /*
     * Revision suffix must contain decimal digits only.
     */
    cursor = number_start;

    while (*cursor != '\0')
    {
        if (isdigit(
                (unsigned char)*cursor) == 0)
        {
            return -1;
        }

        cursor++;
    }

    /*
     * Prevent alternate numeric spellings such as R00.
     *
     * R0 is valid.
     * R1, R2, ... are valid.
     */
    if (number_start[0] == '0' &&
        number_start[1] != '\0')
    {
        return -1;
    }

    end_pointer = NULL;

    number =
        strtoul(
            number_start,
            &end_pointer,
            10
        );

    if (end_pointer == NULL ||
        *end_pointer != '\0')
    {
        return -1;
    }

    *revision_number = number;

    return 0;
}

/**
 * @brief Validates Previous Revision against revision number.
 *
 * @param record Policy index record.
 * @param revision_number Parsed current revision number.
 *
 * @return 0 on success, non-zero on failure.
 */
static int digit_policy_index_validate_previous(
    const digit_policy_index_record_t *record,
    unsigned long revision_number)
{
    char expected[
        DIGIT_POLICY_INDEX_PREVIOUS_MAX
    ];

    int written;

    if (record == NULL)
    {
        return -1;
    }

    /*
     * Original publication has no predecessor.
     */
    if (revision_number == 0UL)
    {
        if (strcmp(
                record->previous_revision,
                "NONE") != 0)
        {
            return -1;
        }

        return 0;
    }

    /*
     * Later revisions identify their immediate predecessor.
     */
    written =
        snprintf(
            expected,
            sizeof(expected),
            "%s.R%lu",
            record->root_document_id,
            revision_number - 1UL
        );

    if (written < 0 ||
        (size_t)written >= sizeof(expected))
    {
        return -1;
    }

    if (strcmp(
            record->previous_revision,
            expected) != 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Validates one record before corpus-level ambiguity checks.
 *
 * @param record Policy index record.
 * @param result Receives record result.
 */
static void digit_policy_index_validate_record(
    const digit_policy_index_record_t *record,
    digit_policy_index_validation_entry_t *result)
{
    int revision_result;
    unsigned long revision_number;

    if (record == NULL ||
        result == NULL)
    {
        return;
    }

    memset(
        result,
        0,
        sizeof(*result)
    );

    result->state =
        DIGIT_POLICY_INDEX_INVALID;

    if (record->root_document_id[0] == '\0')
    {
        result->reason =
            DIGIT_POLICY_INDEX_REASON_ROOT_ID_INVALID;

        return;
    }

    revision_number = 0UL;

    revision_result =
        digit_policy_index_parse_revision(
            record->root_document_id,
            record->revision_id,
            &revision_number
        );

    if (revision_result == -2)
    {
        result->reason =
            DIGIT_POLICY_INDEX_REASON_REVISION_ROOT_MISMATCH;

        return;
    }

    if (revision_result != 0)
    {
        result->reason =
            DIGIT_POLICY_INDEX_REASON_REVISION_ID_INVALID;

        return;
    }

    result->revision_number =
        revision_number;

    if (record->previous_revision[0] == '\0')
    {
        result->reason =
            DIGIT_POLICY_INDEX_REASON_PREVIOUS_REVISION_INVALID;

        return;
    }

    if (digit_policy_index_validate_previous(
            record,
            revision_number) != 0)
    {
        result->reason =
            DIGIT_POLICY_INDEX_REASON_PREVIOUS_REVISION_MISMATCH;

        return;
    }

    if (digit_policy_index_validate_sha256(
            record->sha256) == 0)
    {
        result->reason =
            DIGIT_POLICY_INDEX_REASON_SHA256_INVALID;

        return;
    }

    result->state =
        DIGIT_POLICY_INDEX_VALID;

    result->reason =
        DIGIT_POLICY_INDEX_REASON_NONE;
}

/**
 * @brief Applies duplicate identity detection.
 *
 * Duplicate root or revision identities produce ambiguity.
 *
 * @param index Policy index.
 * @param report Validation report.
 */
static void digit_policy_index_detect_duplicates(
    const digit_policy_index_t *index,
    digit_policy_index_validation_report_t *report)
{
    size_t left;
    size_t right;

    if (index == NULL ||
        report == NULL)
    {
        return;
    }

    for (left = 0U;
         left < index->record_count;
         left++)
    {
        for (right = left + 1U;
             right < index->record_count;
             right++)
        {
            if (strcmp(
                    index->records[left].root_document_id,
                    index->records[right].root_document_id) == 0)
            {
                report->entries[left].state =
                    DIGIT_POLICY_INDEX_AMBIGUOUS;

                report->entries[left].reason =
                    DIGIT_POLICY_INDEX_REASON_DUPLICATE_ROOT_ID;

                report->entries[right].state =
                    DIGIT_POLICY_INDEX_AMBIGUOUS;

                report->entries[right].reason =
                    DIGIT_POLICY_INDEX_REASON_DUPLICATE_ROOT_ID;
            }

            if (strcmp(
                    index->records[left].revision_id,
                    index->records[right].revision_id) == 0)
            {
                report->entries[left].state =
                    DIGIT_POLICY_INDEX_AMBIGUOUS;

                report->entries[left].reason =
                    DIGIT_POLICY_INDEX_REASON_DUPLICATE_REVISION_ID;

                report->entries[right].state =
                    DIGIT_POLICY_INDEX_AMBIGUOUS;

                report->entries[right].reason =
                    DIGIT_POLICY_INDEX_REASON_DUPLICATE_REVISION_ID;
            }
        }
    }
}

int digit_policy_index_validate(
    const digit_policy_index_t *index,
    digit_policy_index_validation_report_t *report)
{
    size_t record_index;

    if (index == NULL ||
        report == NULL)
    {
        return -1;
    }

    memset(
        report,
        0,
        sizeof(*report)
    );

    if (index->record_count >
        DIGIT_POLICY_INDEX_MAX_RECORDS)
    {
        return -1;
    }

    report->entry_count =
        index->record_count;

    /*
     * First pass:
     *
     * Validate each record independently.
     */
    for (record_index = 0U;
         record_index < index->record_count;
         record_index++)
    {
        digit_policy_index_validate_record(
            &index->records[record_index],
            &report->entries[record_index]
        );
    }

    /*
     * Second pass:
     *
     * Detect corpus-level identity ambiguity.
     */
    digit_policy_index_detect_duplicates(
        index,
        report
    );

    /*
     * Final pass:
     *
     * Produce deterministic summary counts.
     */
    for (record_index = 0U;
         record_index < report->entry_count;
         record_index++)
    {
        switch (report->entries[record_index].state)
        {
            case DIGIT_POLICY_INDEX_VALID:
                report->valid_count++;
                break;

            case DIGIT_POLICY_INDEX_INVALID:
                report->invalid_count++;
                break;

            case DIGIT_POLICY_INDEX_AMBIGUOUS:
                report->ambiguous_count++;
                break;

            default:
                report->invalid_count++;
                break;
        }
    }

    return 0;
}