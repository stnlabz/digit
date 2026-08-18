/**
 * @file digit_company_preservation.c
 * @brief Company Preservation implementation for STN-LABZ Digit Core.
 *
 * Company Preservation temporarily supersedes ordinary mission execution.
 *
 * Existing mission state is preserved rather than replaced.
 */

#include <stddef.h>
#include <string.h>

#include "digit_audit.h"
#include "digit_company_preservation.h"
#include "digit_mission.h"

/**
 * @brief Core-owned Company Preservation state.
 */
static digit_company_preservation_t
    g_digit_company_preservation;

/**
 * @brief Indicates whether the component is initialized.
 */
static int g_digit_company_preservation_initialized = 0;

int digit_company_preservation_init(void)
{
    if (g_digit_company_preservation_initialized != 0)
    {
        return -1;
    }

    memset(
        &g_digit_company_preservation,
        0,
        sizeof(g_digit_company_preservation)
    );

    g_digit_company_preservation.state =
        DIGIT_COMPANY_PRESERVATION_INACTIVE;

    g_digit_company_preservation_initialized = 1;

    return 0;
}

int digit_company_preservation_enter(
    const char *reason)
{
    const digit_mission_t *mission;
    size_t length;

    if (g_digit_company_preservation_initialized == 0 ||
        reason == NULL)
    {
        return -1;
    }

    if (g_digit_company_preservation.state ==
        DIGIT_COMPANY_PRESERVATION_ACTIVE)
    {
        return -1;
    }

    length =
        strlen(reason);

    if (length == 0U ||
        length > DIGIT_COMPANY_PRESERVATION_REASON_MAX)
    {
        return -1;
    }

    /*
     * Preserve the currently assigned mission.
     *
     * If ordinary mission execution is ACTIVE, suspend it before
     * Company Preservation takes priority.
     */
    mission =
        digit_mission_get();

    if (mission != NULL &&
        mission->state == DIGIT_MISSION_ACTIVE)
    {
        if (digit_mission_suspend() != 0)
        {
            return -1;
        }
    }

    memset(
        g_digit_company_preservation.reason,
        0,
        sizeof(g_digit_company_preservation.reason)
    );

    memcpy(
        g_digit_company_preservation.reason,
        reason,
        length
    );

    g_digit_company_preservation.reason[length] =
        '\0';

    /*
     * Establish preservation state before attempting supporting audit.
     *
     * Audit failure must not prevent the fail-safe state from becoming
     * active.
     */
    g_digit_company_preservation.state =
        DIGIT_COMPANY_PRESERVATION_ACTIVE;

    (void)digit_audit_append(
        "COMPANY_PRESERVATION",
        "Company Preservation activated."
    );

    return 0;
}

int digit_company_preservation_release(
    int authorized_release)
{
    if (g_digit_company_preservation_initialized == 0)
    {
        return -1;
    }

    if (g_digit_company_preservation.state !=
        DIGIT_COMPANY_PRESERVATION_ACTIVE)
    {
        return -1;
    }

    /*
     * Digit cannot independently release Company Preservation.
     */
    if (authorized_release == 0)
    {
        (void)digit_audit_append(
            "COMPANY_PRESERVATION",
            "Company Preservation release denied."
        );

        return -1;
    }

    (void)digit_audit_append(
        "COMPANY_PRESERVATION",
        "Authorized Company Preservation release accepted."
    );

    memset(
        g_digit_company_preservation.reason,
        0,
        sizeof(g_digit_company_preservation.reason)
    );

    g_digit_company_preservation.state =
        DIGIT_COMPANY_PRESERVATION_INACTIVE;

    /*
     * A previously suspended mission remains suspended.
     *
     * Resumption requires a separate explicit mission transition.
     */
    return 0;
}

const digit_company_preservation_t *
digit_company_preservation_get(void)
{
    if (g_digit_company_preservation_initialized == 0)
    {
        return NULL;
    }

    return &g_digit_company_preservation;
}

int digit_company_preservation_is_active(void)
{
    if (g_digit_company_preservation_initialized == 0)
    {
        return 0;
    }

    return
        g_digit_company_preservation.state ==
        DIGIT_COMPANY_PRESERVATION_ACTIVE;
}

int digit_company_preservation_is_ready(void)
{
    return g_digit_company_preservation_initialized;
}

void digit_company_preservation_shutdown(void)
{
    memset(
        &g_digit_company_preservation,
        0,
        sizeof(g_digit_company_preservation)
    );

    g_digit_company_preservation_initialized = 0;
}