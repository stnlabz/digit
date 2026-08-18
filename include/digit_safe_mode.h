/**
 * @file digit_safe_mode.h
 * @brief Safe Mode interface for STN-LABZ Digit Core.
 *
 * Safe Mode is part of Digit's Core safety nucleus.
 *
 * Digit may enter Safe Mode when a condition requires affected operation
 * to cease. Safe Mode remains active until an authorized release is
 * explicitly provided.
 */

#ifndef STN_LABZ_DIGIT_SAFE_MODE_H
#define STN_LABZ_DIGIT_SAFE_MODE_H

/**
 * @brief Maximum Safe Mode reason length excluding null termination.
 */
#define DIGIT_SAFE_MODE_REASON_MAX 256U

/**
 * @brief Runtime Safe Mode state.
 */
typedef enum digit_safe_mode_state
{
    DIGIT_SAFE_MODE_INACTIVE = 0,
    DIGIT_SAFE_MODE_ACTIVE
} digit_safe_mode_state_t;

/**
 * @brief Current Safe Mode information.
 */
typedef struct digit_safe_mode
{
    digit_safe_mode_state_t state;

    char reason[
        DIGIT_SAFE_MODE_REASON_MAX + 1U
    ];

} digit_safe_mode_t;

/**
 * @brief Initializes Safe Mode state.
 *
 * Digit begins with Safe Mode inactive.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_safe_mode_init(void);

/**
 * @brief Enters Safe Mode.
 *
 * Entering Safe Mode does not require prior authorization. A detected
 * condition requiring fail-safe behavior may cause Core to enter Safe Mode.
 *
 * The first active Safe Mode reason is preserved. Re-entry while Safe Mode
 * is already active is rejected rather than silently replacing evidence.
 *
 * @param reason Non-empty reason for entering Safe Mode.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_safe_mode_enter(
    const char *reason
);

/**
 * @brief Releases Safe Mode.
 *
 * Safe Mode release requires explicit established authorization supplied
 * by the applicable authority boundary.
 *
 * @param authorized_release Non-zero only when release authority has been
 *        explicitly established.
 *
 * @return 0 on successful release, non-zero when release is denied or the
 *         transition is invalid.
 */
int digit_safe_mode_release(
    int authorized_release
);

/**
 * @brief Returns current Safe Mode information.
 *
 * @return Read-only Safe Mode pointer, or NULL when uninitialized.
 */
const digit_safe_mode_t *digit_safe_mode_get(void);

/**
 * @brief Determines whether Safe Mode is active.
 *
 * @return 1 when Safe Mode is active, otherwise 0.
 */
int digit_safe_mode_is_active(void);

/**
 * @brief Determines whether the Safe Mode component is initialized.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_safe_mode_is_ready(void);

/**
 * @brief Clears runtime Safe Mode state during controlled Core shutdown.
 */
void digit_safe_mode_shutdown(void);

#endif /* STN_LABZ_DIGIT_SAFE_MODE_H */