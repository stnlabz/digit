/**
 * @file digit_mission.c
 * @brief Runtime mission-state implementation for STN-LABZ Digit Core.
 *
 * Core owns mission state and permits only explicitly defined state
 * transitions.
 *
 * This component does not determine who may assign a mission.
 * Human-authority enforcement remains a separate Core boundary.
 */

#include <string.h>

#include "digit_mission.h"

/**
 * @brief Core-owned mission state.
 */
static digit_mission_t g_digit_mission;

/**
 * @brief Indicates whether mission state is initialized.
 */
static int g_digit_mission_initialized = 0;

int digit_mission_init(void)
{
    if (g_digit_mission_initialized != 0)
    {
        return -1;
    }

    memset(
        &g_digit_mission,
        0,
        sizeof(g_digit_mission)
    );

    g_digit_mission.state =
        DIGIT_MISSION_NONE;

    g_digit_mission_initialized = 1;

    return 0;
}

int digit_mission_assign(
    const char *mission_text)
{
    size_t length;

    if (g_digit_mission_initialized == 0 ||
        mission_text == NULL)
    {
        return -1;
    }

    if (g_digit_mission.state !=
            DIGIT_MISSION_NONE &&
        g_digit_mission.state !=
            DIGIT_MISSION_COMPLETED)
    {
        return -1;
    }

    length =
        strlen(mission_text);

    if (length == 0U ||
        length > DIGIT_MISSION_TEXT_MAX)
    {
        return -1;
    }

    /*
     * Clear previous mission data before establishing a new assignment.
     */
    memset(
        g_digit_mission.text,
        0,
        sizeof(g_digit_mission.text)
    );

    memcpy(
        g_digit_mission.text,
        mission_text,
        length
    );

    g_digit_mission.text[length] = '\0';

    g_digit_mission.state =
        DIGIT_MISSION_ASSIGNED;

    return 0;
}

int digit_mission_activate(void)
{
    if (g_digit_mission_initialized == 0)
    {
        return -1;
    }

    if (g_digit_mission.state !=
        DIGIT_MISSION_ASSIGNED)
    {
        return -1;
    }

    g_digit_mission.state =
        DIGIT_MISSION_ACTIVE;

    return 0;
}

int digit_mission_suspend(void)
{
    if (g_digit_mission_initialized == 0)
    {
        return -1;
    }

    if (g_digit_mission.state !=
        DIGIT_MISSION_ACTIVE)
    {
        return -1;
    }

    g_digit_mission.state =
        DIGIT_MISSION_SUSPENDED;

    return 0;
}

int digit_mission_resume(void)
{
    if (g_digit_mission_initialized == 0)
    {
        return -1;
    }

    if (g_digit_mission.state !=
        DIGIT_MISSION_SUSPENDED)
    {
        return -1;
    }

    g_digit_mission.state =
        DIGIT_MISSION_ACTIVE;

    return 0;
}

int digit_mission_complete(void)
{
    if (g_digit_mission_initialized == 0)
    {
        return -1;
    }

    if (g_digit_mission.state !=
        DIGIT_MISSION_ACTIVE)
    {
        return -1;
    }

    g_digit_mission.state =
        DIGIT_MISSION_COMPLETED;

    return 0;
}

const digit_mission_t *digit_mission_get(void)
{
    if (g_digit_mission_initialized == 0)
    {
        return NULL;
    }

    return &g_digit_mission;
}

int digit_mission_is_ready(void)
{
    return g_digit_mission_initialized;
}

void digit_mission_shutdown(void)
{
    memset(
        &g_digit_mission,
        0,
        sizeof(g_digit_mission)
    );

    g_digit_mission_initialized = 0;
}