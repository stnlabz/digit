/**
 * @file digit_authority.c
 * @brief Human-authority boundary implementation for STN-LABZ Digit Core.
 *
 * Digit cannot grant human authority to herself.
 *
 * Human authorization must be established by an applicable external
 * human-controlled process before Digit may represent that authorization
 * as granted.
 */

#include <string.h>

#include "digit_authority.h"

/**
 * @brief Indicates whether the authority boundary is active.
 */
static int g_digit_authority_initialized = 0;

int digit_authority_init(void)
{
    if (g_digit_authority_initialized != 0)
    {
        return -1;
    }

    g_digit_authority_initialized = 1;

    return 0;
}

int digit_authority_evaluate(
    int authority_required,
    int human_approved,
    digit_authority_result_t *result)
{
    if (result == NULL)
    {
        return -1;
    }

    if (g_digit_authority_initialized == 0)
    {
        return -1;
    }

    memset(
        result,
        0,
        sizeof(*result)
    );

    result->state =
        DIGIT_AUTHORITY_UNKNOWN;

    /*
     * No human-authority boundary applies to this operation.
     */
    if (authority_required == 0)
    {
        result->human_authority_required = 0;
        result->human_authority_established = 0;

        result->state =
            DIGIT_AUTHORITY_NOT_REQUIRED;

        return 0;
    }

    result->human_authority_required = 1;

    /*
     * Human authority is required but has not been established.
     *
     * Digit does not infer approval.
     */
    if (human_approved == 0)
    {
        result->human_authority_established = 0;

        result->state =
            DIGIT_AUTHORITY_REQUIRED;

        return 0;
    }

    /*
     * Explicit human authorization has been established by the
     * applicable authorized process.
     */
    result->human_authority_established = 1;

    result->state =
        DIGIT_AUTHORITY_GRANTED;

    return 0;
}

int digit_authority_is_ready(void)
{
    return g_digit_authority_initialized;
}

void digit_authority_shutdown(void)
{
    g_digit_authority_initialized = 0;
}