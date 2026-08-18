/**
 * @file digit_safe_mode.c
 * @brief Safe Mode implementation for STN-LABZ Digit Core.
 *
 * Safe Mode provides a deterministic fail-safe runtime state.
 *
 * Entry is permitted when Core detects a condition requiring affected
 * operation to cease.
 *
 * Exit requires explicitly established authorization.
 */

#include <stddef.h>
#include <string.h>

#include "digit_audit.h"
#include "digit_safe_mode.h"

/**
 * @brief Core-owned Safe Mode state.
 */
static digit_safe_mode_t g_digit_safe_mode;

/**
 * @brief Indicates whether Safe Mode has been initialized.
 */
static int g_digit_safe_mode_initialized = 0;

int digit_safe_mode_init(void)
{
    if (g_digit_safe_mode_initialized != 0)
    {
        return -1;
    }

    memset(
        &g_digit_safe_mode,
        0,
        sizeof(g_digit_safe_mode)
    );

    g_digit_safe_mode.state =
        DIGIT_SAFE_MODE_INACTIVE;

    g_digit_safe_mode_initialized = 1;

    return 0;
}

int digit_safe_mode_enter(
    const char *reason)
{
    size_t length;

    if (g_digit_safe_mode_initialized == 0 ||
        reason == NULL)
    {
        return -1;
    }

    /*
     * Do not replace the established reason for an already-active
     * Safe Mode condition.
     */
    if (g_digit_safe_mode.state ==
        DIGIT_SAFE_MODE_ACTIVE)
    {
        return -1;
    }

    length =
        strlen(reason);

    if (length == 0U ||
        length > DIGIT_SAFE_MODE_REASON_MAX)
    {
        return -1;
    }

    memset(
        g_digit_safe_mode.reason,
        0,
        sizeof(g_digit_safe_mode.reason)
    );

    memcpy(
        g_digit_safe_mode.reason,
        reason,
        length
    );

    g_digit_safe_mode.reason[length] =
        '\0';

    /*
     * Establish fail-safe state before attempting supporting audit.
     *
     * Failure to record evidence must never prevent entry into Safe Mode.
     */
    g_digit_safe_mode.state =
        DIGIT_SAFE_MODE_ACTIVE;

    (void)digit_audit_append(
        "SAFE_MODE",
        "Safe Mode entered."
    );

    return 0;
}

int digit_safe_mode_release(
    int authorized_release)
{
    if (g_digit_safe_mode_initialized == 0)
    {
        return -1;
    }

    if (g_digit_safe_mode.state !=
        DIGIT_SAFE_MODE_ACTIVE)
    {
        return -1;
    }

    /*
     * Digit cannot release herself from Safe Mode without explicitly
     * established authorization.
     */
    if (authorized_release == 0)
    {
        (void)digit_audit_append(
            "SAFE_MODE",
            "Safe Mode release denied."
        );

        return -1;
    }

    (void)digit_audit_append(
        "SAFE_MODE",
        "Authorized Safe Mode release accepted."
    );

    memset(
        g_digit_safe_mode.reason,
        0,
        sizeof(g_digit_safe_mode.reason)
    );

    g_digit_safe_mode.state =
        DIGIT_SAFE_MODE_INACTIVE;

    return 0;
}

const digit_safe_mode_t *digit_safe_mode_get(void)
{
    if (g_digit_safe_mode_initialized == 0)
    {
        return NULL;
    }

    return &g_digit_safe_mode;
}

int digit_safe_mode_is_active(void)
{
    if (g_digit_safe_mode_initialized == 0)
    {
        return 0;
    }

    return
        g_digit_safe_mode.state ==
        DIGIT_SAFE_MODE_ACTIVE;
}

int digit_safe_mode_is_ready(void)
{
    return g_digit_safe_mode_initialized;
}

void digit_safe_mode_shutdown(void)
{
    memset(
        &g_digit_safe_mode,
        0,
        sizeof(g_digit_safe_mode)
    );

    g_digit_safe_mode_initialized = 0;
}