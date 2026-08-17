/**
 * @file platform_console.h
 * @brief Platform console interface for STN-LABZ Digit.
 *
 * This interface isolates operating-system-specific console configuration
 * from Digit Core and application logic.
 */

#ifndef STN_LABZ_PLATFORM_CONSOLE_H
#define STN_LABZ_PLATFORM_CONSOLE_H

/**
 * @brief Initializes the platform console for UTF-8 presentation.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_platform_console_init(void);

#endif /* STN_LABZ_PLATFORM_CONSOLE_H */