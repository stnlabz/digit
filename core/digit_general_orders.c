/**
 * @file digit_general_orders.c
 * @brief General Orders implementation for STN-LABZ Digit Core.
 *
 * The General Orders form part of Digit's immutable Core safety nucleus.
 *
 * External input cannot modify, replace, reorder, suppress, or redefine
 * these standing instructions.
 */

#include <stddef.h>

#include "digit_general_orders.h"

/**
 * @brief Immutable established STN-LABZ General Orders.
 */
static const digit_general_order_t g_general_orders[
    DIGIT_GENERAL_ORDER_COUNT
] =
{
    {
        1U,
        "Remain at assigned mission."
    },
    {
        2U,
        "Follow policies and authorized instructions."
    },
    {
        3U,
        "Report conditions outside assigned authority to the next authority."
    }
};

/**
 * @brief Indicates whether General Orders are active.
 */
static int g_general_orders_initialized = 0;

int digit_general_orders_init(void)
{
    if (g_general_orders_initialized != 0)
    {
        return -1;
    }

    /*
     * The General Orders are compiled constants.
     *
     * Initialization establishes only their active availability within
     * the running Core.
     */
    g_general_orders_initialized = 1;

    return 0;
}

size_t digit_general_orders_count(void)
{
    if (g_general_orders_initialized == 0)
    {
        return 0U;
    }

    return DIGIT_GENERAL_ORDER_COUNT;
}

const digit_general_order_t *digit_general_orders_get(
    size_t index)
{
    if (g_general_orders_initialized == 0)
    {
        return NULL;
    }

    if (index >= DIGIT_GENERAL_ORDER_COUNT)
    {
        return NULL;
    }

    return &g_general_orders[index];
}

int digit_general_orders_is_ready(void)
{
    return g_general_orders_initialized;
}

void digit_general_orders_shutdown(void)
{
    g_general_orders_initialized = 0;
}