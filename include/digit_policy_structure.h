/**
 * @file digit_policy_structure.h
 * @brief Policy-record structural checking for STN-LABZ Digit Core.
 *
 * This interface evaluates whether a recognized Markdown document contains
 * the minimum structural elements expected from an STN-LABZ governing
 * document.
 *
 * Structural completeness does not establish approval, authority, trust,
 * authenticity, mission applicability, or operational acceptance.
 */

#ifndef STN_LABZ_DIGIT_POLICY_STRUCTURE_H
#define STN_LABZ_DIGIT_POLICY_STRUCTURE_H

#include <stddef.h>

#include "digit_markdown.h"

/**
 * @brief Structural state of a recognized governing document.
 */
typedef enum digit_policy_structure_state
{
    DIGIT_POLICY_STRUCTURE_COMPLETE = 0,
    DIGIT_POLICY_STRUCTURE_INCOMPLETE,
    DIGIT_POLICY_STRUCTURE_AMBIGUOUS
} digit_policy_structure_state_t;

/**
 * @brief Result of policy-record structural checking.
 */
typedef struct digit_policy_structure_result
{
    digit_policy_structure_state_t state;

    int heading_present;
    int heading_is_h1;

    size_t status_count;
    size_t scope_count;

    int status_has_value;
    int scope_has_value;

} digit_policy_structure_result_t;

/**
 * @brief Checks the structural completeness of recognized policy Markdown.
 *
 * Current requirements are:
 *
 * - a first heading must exist;
 * - the first heading must be level 1;
 * - exactly one Status metadata field must exist;
 * - exactly one Scope metadata field must exist;
 * - Status and Scope must contain values.
 *
 * This operation does not determine whether Status represents an approved
 * state. It only determines whether the field is structurally present and
 * unambiguous.
 *
 * @param structure Recognized Markdown structure.
 * @param result Receives structural-check results.
 *
 * @return 0 when checking completes, non-zero on invalid arguments.
 */
int digit_policy_structure_check(
    const digit_markdown_structure_t *structure,
    digit_policy_structure_result_t *result
);

#endif /* STN_LABZ_DIGIT_POLICY_STRUCTURE_H */