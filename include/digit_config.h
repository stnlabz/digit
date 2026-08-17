/**
 * @file digit_config.h
 * @brief Core configuration interface for STN-LABZ Digit.
 *
 * This interface defines the configuration values required by Digit Core.
 * External configuration loading will be implemented separately.
 */

#ifndef STN_LABZ_DIGIT_CONFIG_H
#define STN_LABZ_DIGIT_CONFIG_H

#include <stddef.h>

/**
 * @brief Maximum supported path length for configured Digit directories.
 */
#define DIGIT_CONFIG_PATH_MAX 512U

/**
 * @brief Represents Digit Core configuration.
 */
typedef struct digit_config
{
    char policy_directory[DIGIT_CONFIG_PATH_MAX];
    char knowledge_directory[DIGIT_CONFIG_PATH_MAX];
} digit_config_t;

/**
 * @brief Initializes configuration with deterministic defaults.
 *
 * @param config Configuration object to initialize.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_config_init(digit_config_t *config);

/**
 * @brief Returns the active Digit Core configuration.
 *
 * @return Pointer to the active configuration, or NULL if configuration
 *         has not been initialized.
 */
const digit_config_t *digit_config_get(void);

/**
 * @brief Clears the active configuration state.
 */
void digit_config_shutdown(void);

#endif /* STN_LABZ_DIGIT_CONFIG_H */