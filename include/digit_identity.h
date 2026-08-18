/**
 * @file digit_identity.h
 * @brief Core identity interface for STN-LABZ Digit.
 *
 * Digit identity is part of the Core safety nucleus.
 *
 * Identity establishes who the running system is.
 * It does not independently establish mission authority, operational
 * authority, knowledge authorization, capability, or qualification.
 */

#ifndef STN_LABZ_DIGIT_IDENTITY_H
#define STN_LABZ_DIGIT_IDENTITY_H

/**
 * @brief Immutable Digit Core identity.
 */
typedef struct digit_identity
{
    const char *system_name;
    const char *organization;
    const char *role;

} digit_identity_t;

/**
 * @brief Initializes Digit Core identity.
 *
 * Identity initialization is deterministic and does not depend upon
 * external files, network services, retrieval systems, or runtime input.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_identity_init(void);

/**
 * @brief Returns the active Digit Core identity.
 *
 * @return Read-only pointer to Digit identity, or NULL when identity
 *         has not been initialized.
 */
const digit_identity_t *digit_identity_get(void);

/**
 * @brief Clears active identity state.
 *
 * Immutable identity constants remain compiled into the implementation,
 * while active identity availability is withdrawn during shutdown.
 */
void digit_identity_shutdown(void);

#endif /* STN_LABZ_DIGIT_IDENTITY_H */