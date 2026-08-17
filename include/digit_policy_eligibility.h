/**
 * @file digit_policy_eligibility.h
 * @brief Policy eligibility interface for STN-LABZ Digit Core.
 *
 * This interface determines whether a governing document is eligible
 * to proceed to later authority, provenance, and operational validation.
 *
 * Every eligibility decision includes an explicit reason.
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
 * @brief Deterministic reason for an eligibility decision.
 */
typedef enum digit_policy_eligibility_reason
{
    DIGIT_POLICY_ELIGIBILITY_REASON_NONE = 0,

    DIGIT_POLICY_ELIGIBILITY_REASON_STRUCTURE_INCOMPLETE,

    DIGIT_POLICY_ELIGIBILITY_REASON_STRUCTURE_AMBIGUOUS,

    DIGIT_POLICY_ELIGIBILITY_REASON_STATUS_MISSING,

    DIGIT_POLICY_ELIGIBILITY_REASON_STATUS_NOT_APPROVED

} digit_policy_eligibility_reason_t;

/**
 * @brief Result of policy eligibility evaluation.
 */
typedef struct digit_policy_eligibility_result
{
    digit_policy_eligibility_state_t state;

    digit_policy_eligibility_reason_t reason;

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
 * Every completed evaluation produces both an eligibility state and
 * an explicit reason.
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