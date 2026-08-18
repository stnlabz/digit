/**
 * @file digit_identity.c
 * @brief Core identity implementation for STN-LABZ Digit.
 *
 * Digit identity is deterministic and compiled into Core.
 *
 * Identity is not loaded from configuration, policy files, RAG content,
 * environment variables, command-line arguments, or external systems.
 *
 * No external input may redefine who Digit is.
 */

#include <stddef.h>

#include "digit_identity.h"

/**
 * @brief Immutable compiled Digit identity.
 */
static const digit_identity_t g_digit_identity =
{
    "Digit",
    "STN-LABZ",
    "Private Engineering Intelligence"
};

/**
 * @brief Indicates whether Core identity is active.
 */
static int g_digit_identity_initialized = 0;

int digit_identity_init(void)
{
    if (g_digit_identity_initialized != 0)
    {
        return -1;
    }

    g_digit_identity_initialized = 1;

    return 0;
}

const digit_identity_t *digit_identity_get(void)
{
    if (g_digit_identity_initialized == 0)
    {
        return NULL;
    }

    return &g_digit_identity;
}

void digit_identity_shutdown(void)
{
    g_digit_identity_initialized = 0;
}