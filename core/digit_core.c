/**
 * @file digit_core.c
 * @brief Lifecycle implementation for the STN-LABZ Digit Core.
 *
 * Digit Core owns the system safety nucleus and controls initialization
 * and shutdown of Core components.
 *
 * Current initialized Core components:
 *
 * - Core identity;
 * - General Orders;
 * - human-authority boundary;
 * - deterministic configuration.
 */

#include "digit_core.h"
#include "digit_authority.h"
#include "digit_config.h"
#include "digit_general_orders.h"
#include "digit_identity.h"

/**
 * @brief Current Core lifecycle state.
 */
static digit_core_state_t g_digit_core_state =
    DIGIT_CORE_UNINITIALIZED;

/**
 * @brief Core configuration instance.
 */
static digit_config_t g_core_config;

int digit_core_init(void)
{
    if (g_digit_core_state !=
            DIGIT_CORE_UNINITIALIZED &&
        g_digit_core_state !=
            DIGIT_CORE_STOPPED)
    {
        return -1;
    }

    g_digit_core_state =
        DIGIT_CORE_INITIALIZING;

    /*
     * Identity initializes first.
     */
    if (digit_identity_init() != 0)
    {
        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Standing General Orders initialize immediately after identity.
     */
    if (digit_general_orders_init() != 0)
    {
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Human-authority boundary becomes active before configurable or
     * mission-specific processing begins.
     */
    if (digit_authority_init() != 0)
    {
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Initialize deterministic Core configuration.
     */
    if (digit_config_init(
            &g_core_config) != 0)
    {
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Core becomes READY only after all currently required Core
     * initialization boundaries have succeeded.
     */
    g_digit_core_state =
        DIGIT_CORE_READY;

    return 0;
}

digit_core_state_t digit_core_get_state(void)
{
    return g_digit_core_state;
}

void digit_core_shutdown(void)
{
    if (g_digit_core_state ==
            DIGIT_CORE_UNINITIALIZED ||
        g_digit_core_state ==
            DIGIT_CORE_STOPPED)
    {
        return;
    }

    g_digit_core_state =
        DIGIT_CORE_SHUTTING_DOWN;

    /*
     * Shutdown occurs in reverse initialization order.
     */
    digit_config_shutdown();
    digit_authority_shutdown();
    digit_general_orders_shutdown();
    digit_identity_shutdown();

    g_digit_core_state =
        DIGIT_CORE_STOPPED;
}