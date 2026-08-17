/**
 * @file digit_policy_eligibility.c
 * @brief Policy eligibility implementation for STN-LABZ Digit Core.
 *
 * This component determines whether a governing document is eligible
 * to proceed to later validation stages.
 *
 * Every completed decision preserves an explicit reason.
 *
 * Eligibility does not establish trust, authority, authenticity,
 * provenance, mission applicability, or operational acceptance.
 */

#include <string.h>

#include "digit_policy_eligibility.h"

int digit_policy_eligibility_check(
    const digit_markdown_structure_t *structure,
    const digit_policy_structure_result_t *structure_result,
    digit_policy_eligibility_result_t *result)
{
    const char *status;

    if (structure == NULL ||
        structure_result == NULL ||
        result == NULL)
    {
        return -1;
    }

    memset(
        result,
        0,
        sizeof(*result)
    );

    /*
     * Default state is denial.
     *
     * A document becomes eligible only after every current
     * eligibility requirement is positively established.
     */
    result->state =
        DIGIT_POLICY_ELIGIBILITY_NOT_ELIGIBLE;

    result->reason =
        DIGIT_POLICY_ELIGIBILITY_REASON_STRUCTURE_INCOMPLETE;

    /*
     * Ambiguous structure receives its own deterministic reason.
     *
     * Digit does not choose between conflicting structural claims.
     */
    if (structure_result->state ==
        DIGIT_POLICY_STRUCTURE_AMBIGUOUS)
    {
        result->reason =
            DIGIT_POLICY_ELIGIBILITY_REASON_STRUCTURE_AMBIGUOUS;

        return 0;
    }

    /*
     * Incomplete or otherwise non-complete structure cannot proceed.
     */
    if (structure_result->state !=
        DIGIT_POLICY_STRUCTURE_COMPLETE)
    {
        result->reason =
            DIGIT_POLICY_ELIGIBILITY_REASON_STRUCTURE_INCOMPLETE;

        return 0;
    }

    result->structure_complete = 1;

    /*
     * Structural validation should already establish exactly one
     * Status field for a COMPLETE document.
     *
     * Eligibility still independently verifies that a usable value
     * can actually be retrieved.
     */
    status =
        digit_markdown_metadata_get(
            structure,
            "Status"
        );

    if (status == NULL ||
        status[0] == '\0')
    {
        result->reason =
            DIGIT_POLICY_ELIGIBILITY_REASON_STATUS_MISSING;

        return 0;
    }

    result->status_present = 1;

    /*
     * Current STN-LABZ eligibility requires the exact approved
     * status value used by the governing corpus.
     *
     * No case folding, aliasing, approximation, or inference is
     * performed.
     */
    if (strcmp(
            status,
            "Approved") != 0)
    {
        result->reason =
            DIGIT_POLICY_ELIGIBILITY_REASON_STATUS_NOT_APPROVED;

        return 0;
    }

    result->status_approved = 1;

    /*
     * Every current eligibility requirement has been positively
     * established.
     */
    result->state =
        DIGIT_POLICY_ELIGIBILITY_ELIGIBLE;

    result->reason =
        DIGIT_POLICY_ELIGIBILITY_REASON_NONE;

    return 0;
}