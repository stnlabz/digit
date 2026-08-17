/**
 * @file digit_core.c
 * @brief Minimal lifecycle implementation for the STN-LABZ Digit core.
 *
 * This file implements the initial Digit Core lifecycle and configuration
 * initialization.
 */

#include "digit_core.h"
#include "digit_config.h"

/**
 * @brief Current core lifecycle state.
 */
static digit_core_state_t g_digit_core_state = DIGIT_CORE_UNINITIALIZED;

/**
 * @brief Core configuration instance.
 */
static digit_config_t g_core_config;

int digit_core_init(void)
{
    if (g_digit_core_state != DIGIT_CORE_UNINITIALIZED &&
        g_digit_core_state != DIGIT_CORE_STOPPED)
    {
        return -1;
    }

    g_digit_core_state = DIGIT_CORE_INITIALIZING;

    if (digit_config_init(&g_core_config) != 0)
    {
        g_digit_core_state = DIGIT_CORE_STOPPED;
        return -1;
    }

    g_digit_core_state = DIGIT_CORE_READY;

    return 0;
}

digit_core_state_t digit_core_get_state(void)
{
    return g_digit_core_state;
}

void digit_core_shutdown(void)
{
    if (g_digit_core_state == DIGIT_CORE_UNINITIALIZED ||
        g_digit_core_state == DIGIT_CORE_STOPPED)
    {
        return;
    }

    g_digit_core_state = DIGIT_CORE_SHUTTING_DOWN;

    digit_config_shutdown();

    g_digit_core_state = DIGIT_CORE_STOPPED;
}