/**
 * @file digit_policy_eligibility.c
 * @brief Policy eligibility implementation for STN-LABZ Digit Core.
 *
 * This component determines whether a structurally complete governing
 * document is eligible to proceed to later validation stages.
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

    result->state =
        DIGIT_POLICY_ELIGIBILITY_NOT_ELIGIBLE;

    /*
     * Eligibility cannot proceed when structural requirements
     * are not complete.
     */
    if (structure_result->state !=
        DIGIT_POLICY_STRUCTURE_COMPLETE)
    {
        return 0;
    }

    result->structure_complete = 1;

    /*
     * Structural validation already established exactly one
     * Status field for a COMPLETE document.
     */
    status =
        digit_markdown_metadata_get(
            structure,
            "Status"
        );

    if (status == NULL ||
        status[0] == '\0')
    {
        return 0;
    }

    result->status_present = 1;

    /*
     * Current STN-LABZ eligibility requires the exact approved
     * status value used by the governing corpus.
     *
     * No case folding, inference, aliasing, or approximation is
     * performed.
     */
    if (strcmp(
            status,
            "Approved") != 0)
    {
        return 0;
    }

    result->status_approved = 1;

    result->state =
        DIGIT_POLICY_ELIGIBILITY_ELIGIBLE;

    return 0;
}