/**
 * @file digit_policy_structure.c
 * @brief Policy-record structural checking for STN-LABZ Digit Core.
 *
 * This component evaluates structural completeness only.
 *
 * A structurally complete document is not automatically approved,
 * authoritative, trusted, authentic, mission-applicable, or operationally
 * accepted.
 */

#include <stddef.h>
#include <string.h>

#include "digit_policy_structure.h"

/**
 * @brief Counts recognized metadata entries using an exact key.
 *
 * @param structure Recognized Markdown structure.
 * @param key Metadata key.
 *
 * @return Number of matching metadata entries.
 */
static size_t digit_policy_structure_metadata_count(
    const digit_markdown_structure_t *structure,
    const char *key)
{
    size_t index;
    size_t count;

    if (structure == NULL ||
        key == NULL)
    {
        return 0U;
    }

    count = 0U;

    for (index = 0U;
         index < structure->metadata_count;
         index++)
    {
        if (strcmp(
                structure->metadata[index].key,
                key) == 0)
        {
            count++;
        }
    }

    return count;
}

/**
 * @brief Determines whether an exact metadata key has a non-empty value.
 *
 * This helper is used only when exactly one matching field is expected.
 *
 * @param structure Recognized Markdown structure.
 * @param key Metadata key.
 *
 * @return 1 when a non-empty value exists, otherwise 0.
 */
static int digit_policy_structure_metadata_has_value(
    const digit_markdown_structure_t *structure,
    const char *key)
{
    const char *value;

    if (structure == NULL ||
        key == NULL)
    {
        return 0;
    }

    value =
        digit_markdown_metadata_get(
            structure,
            key
        );

    if (value == NULL ||
        value[0] == '\0')
    {
        return 0;
    }

    return 1;
}

int digit_policy_structure_check(
    const digit_markdown_structure_t *structure,
    digit_policy_structure_result_t *result)
{
    if (structure == NULL ||
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
     * Establish heading state.
     */
    if (structure->heading_count > 0U &&
        structure->first_heading[0] != '\0')
    {
        result->heading_present = 1;
    }

    if (result->heading_present != 0 &&
        structure->first_heading_level == 1U)
    {
        result->heading_is_h1 = 1;
    }

    /*
     * Establish required metadata occurrence counts.
     */
    result->status_count =
        digit_policy_structure_metadata_count(
            structure,
            "Status"
        );

    result->scope_count =
        digit_policy_structure_metadata_count(
            structure,
            "Scope"
        );

    /*
     * A required field is considered to have a usable structural value only
     * when exactly one occurrence exists and that value is non-empty.
     */
    if (result->status_count == 1U)
    {
        result->status_has_value =
            digit_policy_structure_metadata_has_value(
                structure,
                "Status"
            );
    }

    if (result->scope_count == 1U)
    {
        result->scope_has_value =
            digit_policy_structure_metadata_has_value(
                structure,
                "Scope"
            );
    }

    /*
     * Duplicate required metadata produces ambiguity.
     *
     * Digit does not guess which duplicate field was intended to control.
     */
    if (result->status_count > 1U ||
        result->scope_count > 1U)
    {
        result->state =
            DIGIT_POLICY_STRUCTURE_AMBIGUOUS;

        return 0;
    }

    /*
     * Missing or malformed required structure produces an incomplete
     * record.
     */
    if (result->heading_present == 0 ||
        result->heading_is_h1 == 0 ||
        result->status_count != 1U ||
        result->scope_count != 1U ||
        result->status_has_value == 0 ||
        result->scope_has_value == 0)
    {
        result->state =
            DIGIT_POLICY_STRUCTURE_INCOMPLETE;

        return 0;
    }

    result->state =
        DIGIT_POLICY_STRUCTURE_COMPLETE;

    return 0;
}