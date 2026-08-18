/**
 * @file digit_general_orders.h
 * @brief General Orders interface for STN-LABZ Digit Core.
 *
 * The General Orders are part of Digit's Core safety nucleus.
 *
 * They are immutable standing instructions compiled into Core and are not
 * supplied by configuration, policy files, RAG content, modules, command-line
 * arguments, environment variables, or upstream systems.
 */

#ifndef STN_LABZ_DIGIT_GENERAL_ORDERS_H
#define STN_LABZ_DIGIT_GENERAL_ORDERS_H

#include <stddef.h>

/**
 * @brief Number of established STN-LABZ General Orders.
 */
#define DIGIT_GENERAL_ORDER_COUNT 3U

/**
 * @brief One immutable General Order.
 */
typedef struct digit_general_order
{
    unsigned int number;
    const char *text;

} digit_general_order_t;

/**
 * @brief Initializes the General Orders Core component.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_general_orders_init(void);

/**
 * @brief Returns the number of active General Orders.
 *
 * @return Number of active orders, or 0 when not initialized.
 */
size_t digit_general_orders_count(void);

/**
 * @brief Returns one General Order by zero-based index.
 *
 * @param index Zero-based order index.
 *
 * @return Read-only order pointer, or NULL when unavailable.
 */
const digit_general_order_t *digit_general_orders_get(
    size_t index
);

/**
 * @brief Determines whether the General Orders component is active.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_general_orders_is_ready(void);

/**
 * @brief Withdraws active General Orders state during Core shutdown.
 */
void digit_general_orders_shutdown(void);

#endif /* STN_LABZ_DIGIT_GENERAL_ORDERS_H */