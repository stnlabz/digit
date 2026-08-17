/**
 * @file digit_config.c
 * @brief Core configuration implementation for STN-LABZ Digit.
 *
 * This implementation establishes deterministic configuration defaults.
 * File-based JSON configuration loading will be added separately.
 */

#include <stdio.h>
#include <string.h>

#include "digit_config.h"

/**
 * @brief Active Digit configuration.
 */
static digit_config_t g_digit_config;

/**
 * @brief Indicates whether configuration has been initialized.
 */
static int g_digit_config_initialized = 0;

int digit_config_init(digit_config_t *config)
{
    if (config == NULL)
    {
        return -1;
    }

    if (g_digit_config_initialized != 0)
    {
        return -1;
    }

    memset(config, 0, sizeof(*config));

    if (snprintf(
            config->policy_directory,
            sizeof(config->policy_directory),
            "%s",
            "./policies") < 0)
    {
        return -1;
    }

    if (snprintf(
            config->knowledge_directory,
            sizeof(config->knowledge_directory),
            "%s",
            "./knowledge") < 0)
    {
        return -1;
    }

    g_digit_config = *config;
    g_digit_config_initialized = 1;

    return 0;
}

const digit_config_t *digit_config_get(void)
{
    if (g_digit_config_initialized == 0)
    {
        return NULL;
    }

    return &g_digit_config;
}

void digit_config_shutdown(void)
{
    memset(&g_digit_config, 0, sizeof(g_digit_config));
    g_digit_config_initialized = 0;
}