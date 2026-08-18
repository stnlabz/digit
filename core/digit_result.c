/**
 * @file digit_result.c
 * @brief Result and epistemic-state implementation for STN-LABZ Digit Core.
 *
 * Digit preserves deterministic separation between:
 *
 * - PASS;
 * - FAIL;
 * - UNKNOWN;
 *
 * and between:
 *
 * - ESTABLISHED;
 * - INFERRED;
 * - UNKNOWN.
 *
 * Inference does not silently become established fact.
 */

#include <string.h>

#include "digit_result.h"

/**
 * @brief Indicates whether the result component is active.
 */
static int g_digit_result_initialized = 0;

int digit_result_init(void)
{
    if (g_digit_result_initialized != 0)
    {
        return -1;
    }

    g_digit_result_initialized = 1;

    return 0;
}

int digit_result_set_unknown(
    digit_result_t *result)
{
    if (g_digit_result_initialized == 0 ||
        result == NULL)
    {
        return -1;
    }

    memset(
        result,
        0,
        sizeof(*result)
    );

    result->result =
        DIGIT_RESULT_UNKNOWN;

    result->epistemic =
        DIGIT_EPISTEMIC_UNKNOWN;

    return 0;
}

int digit_result_set_established(
    digit_result_t *result,
    digit_result_state_t state)
{
    if (g_digit_result_initialized == 0 ||
        result == NULL)
    {
        return -1;
    }

    /*
     * UNKNOWN is established through digit_result_set_unknown().
     *
     * This function accepts only deterministic PASS or FAIL.
     */
    if (state != DIGIT_RESULT_PASS &&
        state != DIGIT_RESULT_FAIL)
    {
        return -1;
    }

    memset(
        result,
        0,
        sizeof(*result)
    );

    result->result = state;

    result->epistemic =
        DIGIT_EPISTEMIC_ESTABLISHED;

    return 0;
}

int digit_result_set_inferred(
    digit_result_t *result)
{
    if (g_digit_result_initialized == 0 ||
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
     * Inference alone does not establish an operational PASS or FAIL.
     */
    result->result =
        DIGIT_RESULT_UNKNOWN;

    result->epistemic =
        DIGIT_EPISTEMIC_INFERRED;

    return 0;
}

int digit_result_is_established(
    const digit_result_t *result)
{
    if (g_digit_result_initialized == 0 ||
        result == NULL)
    {
        return 0;
    }

    return
        result->epistemic ==
        DIGIT_EPISTEMIC_ESTABLISHED;
}

int digit_result_is_ready(void)
{
    return g_digit_result_initialized;
}

void digit_result_shutdown(void)
{
    g_digit_result_initialized = 0;
}