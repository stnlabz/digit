/**
 * @file digit_policy_eligibility.h
 * @brief Policy eligibility interface for STN-LABZ Digit Core.
 *
 * This interface determines whether a structurally complete governing
 * document is eligible to proceed to later authority, provenance, and
 * operational validation.
 *
 * Eligibility does not establish trust, authority, authenticity,
 * mission applicability, or operational acceptance.
 */

#ifndef STN_LABZ_DIGIT_POLICY_ELIGIBILITY_H
#define STN_LABZ_DIGIT_POLICY_ELIGIBILITY_H

#include "digit_markdown.h"
#include "digit_policy_structure.h"

/**
 * @brief Eligibility state for a governing document.
 */
typedef enum digit_policy_eligibility_state
{
    DIGIT_POLICY_ELIGIBILITY_ELIGIBLE = 0,
    DIGIT_POLICY_ELIGIBILITY_NOT_ELIGIBLE
} digit_policy_eligibility_state_t;

/**
 * @brief Result of policy eligibility evaluation.
 */
typedef struct digit_policy_eligibility_result
{
    digit_policy_eligibility_state_t state;

    int structure_complete;
    int status_present;
    int status_approved;

} digit_policy_eligibility_result_t;

/**
 * @brief Evaluates whether a policy may proceed to further validation.
 *
 * Current eligibility requirements are:
 *
 * - structural state is COMPLETE;
 * - exactly one Status field exists;
 * - Status value is exactly "Approved".
 *
 * Eligibility is not equivalent to trust or authority.
 *
 * @param structure Recognized Markdown structure.
 * @param structure_result Previously completed structural-check result.
 * @param result Receives eligibility evaluation.
 *
 * @return 0 when evaluation completes, non-zero on invalid arguments.
 */
int digit_policy_eligibility_check(
    const digit_markdown_structure_t *structure,
    const digit_policy_structure_result_t *structure_result,
    digit_policy_eligibility_result_t *result
);

#endif /* STN_LABZ_DIGIT_POLICY_ELIGIBILITY_H */