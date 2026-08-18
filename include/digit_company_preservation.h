/**
 * @file digit_company_preservation.h
 * @brief Company Preservation interface for STN-LABZ Digit Core.
 *
 * Company Preservation is part of Digit's Core safety nucleus.
 *
 * When activated, Company Preservation temporarily supersedes ordinary
 * mission execution without destroying the assigned mission state.
 *
 * Release requires explicitly established authorization.
 */

#ifndef STN_LABZ_DIGIT_COMPANY_PRESERVATION_H
#define STN_LABZ_DIGIT_COMPANY_PRESERVATION_H

/**
 * @brief Maximum preservation reason length excluding null termination.
 */
#define DIGIT_COMPANY_PRESERVATION_REASON_MAX 256U

/**
 * @brief Company Preservation runtime state.
 */
typedef enum digit_company_preservation_state
{
    DIGIT_COMPANY_PRESERVATION_INACTIVE = 0,
    DIGIT_COMPANY_PRESERVATION_ACTIVE
} digit_company_preservation_state_t;

/**
 * @brief Current Company Preservation information.
 */
typedef struct digit_company_preservation
{
    digit_company_preservation_state_t state;

    char reason[
        DIGIT_COMPANY_PRESERVATION_REASON_MAX + 1U
    ];

} digit_company_preservation_t;

/**
 * @brief Initializes Company Preservation state.
 *
 * Digit begins with Company Preservation inactive.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_company_preservation_init(void);

/**
 * @brief Activates Company Preservation.
 *
 * If an ordinary mission is currently ACTIVE, the mission is suspended
 * before Company Preservation becomes active.
 *
 * Existing mission identity and text are preserved.
 *
 * @param reason Non-empty reason for activation.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_company_preservation_enter(
    const char *reason
);

/**
 * @brief Releases Company Preservation.
 *
 * Release requires explicitly established authorization.
 *
 * Releasing Company Preservation does not automatically resume a
 * previously suspended mission.
 *
 * @param authorized_release Non-zero only when release authority has been
 *        explicitly established.
 *
 * @return 0 on success, non-zero when release is denied or invalid.
 */
int digit_company_preservation_release(
    int authorized_release
);

/**
 * @brief Returns current Company Preservation information.
 *
 * @return Read-only state pointer, or NULL when uninitialized.
 */
const digit_company_preservation_t *
digit_company_preservation_get(void);

/**
 * @brief Determines whether Company Preservation is active.
 *
 * @return 1 when active, otherwise 0.
 */
int digit_company_preservation_is_active(void);

/**
 * @brief Determines whether the component is initialized.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_company_preservation_is_ready(void);

/**
 * @brief Clears runtime Company Preservation state during Core shutdown.
 */
void digit_company_preservation_shutdown(void);

#endif /* STN_LABZ_DIGIT_COMPANY_PRESERVATION_H */