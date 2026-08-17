/**
 * @file digit_core.h
 * @brief Public interface for the STN-LABZ Digit core lifecycle.
 *
 * This interface defines the minimal lifecycle required to initialize,
 * inspect, and shut down the Digit core.
 */

#ifndef STN_LABZ_DIGIT_CORE_H
#define STN_LABZ_DIGIT_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents the current Digit core lifecycle state.
 */
typedef enum digit_core_state
{
    DIGIT_CORE_UNINITIALIZED = 0,
    DIGIT_CORE_INITIALIZING,
    DIGIT_CORE_READY,
    DIGIT_CORE_SHUTTING_DOWN,
    DIGIT_CORE_STOPPED
} digit_core_state_t;

/**
 * @brief Initializes the Digit core.
 *
 * Initialization is intentionally minimal at this stage. The function
 * establishes core lifecycle state only. Policy loading, qualification,
 * knowledge handling, and other core responsibilities will be added as
 * separately defined and validated components.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_core_init(void);

/**
 * @brief Returns the current Digit core lifecycle state.
 *
 * @return Current core state.
 */
digit_core_state_t digit_core_get_state(void);

/**
 * @brief Performs a controlled shutdown of the Digit core.
 */
void digit_core_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* STN_LABZ_DIGIT_CORE_H */