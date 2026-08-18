/**
 * @file digit_mission.h
 * @brief Runtime mission-state interface for STN-LABZ Digit Core.
 *
 * Mission state is part of Digit's Core safety nucleus.
 *
 * This component establishes deterministic runtime representation of
 * Digit's assigned mission and controls mission-state transitions.
 */

#ifndef STN_LABZ_DIGIT_MISSION_H
#define STN_LABZ_DIGIT_MISSION_H

#include <stddef.h>

/**
 * @brief Maximum mission text length, excluding null termination.
 */
#define DIGIT_MISSION_TEXT_MAX 1024U

/**
 * @brief Runtime mission state.
 */
typedef enum digit_mission_state
{
    DIGIT_MISSION_NONE = 0,
    DIGIT_MISSION_ASSIGNED,
    DIGIT_MISSION_ACTIVE,
    DIGIT_MISSION_SUSPENDED,
    DIGIT_MISSION_COMPLETED
} digit_mission_state_t;

/**
 * @brief Core-owned runtime mission representation.
 */
typedef struct digit_mission
{
    digit_mission_state_t state;

    char text[
        DIGIT_MISSION_TEXT_MAX + 1U
    ];

} digit_mission_t;

/**
 * @brief Initializes runtime mission state.
 *
 * Initialization begins with no assigned mission.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_mission_init(void);

/**
 * @brief Assigns a new mission.
 *
 * Assignment is permitted only when no mission is currently assigned
 * or after the previous mission has completed.
 *
 * Assignment does not automatically activate the mission.
 *
 * @param mission_text Non-empty mission description.
 *
 * @return 0 on success, non-zero when assignment is rejected.
 */
int digit_mission_assign(
    const char *mission_text
);

/**
 * @brief Activates the currently assigned mission.
 *
 * @return 0 on success, non-zero on invalid transition.
 */
int digit_mission_activate(void);

/**
 * @brief Suspends the active mission.
 *
 * @return 0 on success, non-zero on invalid transition.
 */
int digit_mission_suspend(void);

/**
 * @brief Resumes a suspended mission.
 *
 * @return 0 on success, non-zero on invalid transition.
 */
int digit_mission_resume(void);

/**
 * @brief Marks the active mission complete.
 *
 * @return 0 on success, non-zero on invalid transition.
 */
int digit_mission_complete(void);

/**
 * @brief Returns the current mission.
 *
 * @return Read-only mission pointer, or NULL when the component
 *         is not initialized.
 */
const digit_mission_t *digit_mission_get(void);

/**
 * @brief Determines whether mission state is active in Core.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_mission_is_ready(void);

/**
 * @brief Clears runtime mission state during Core shutdown.
 */
void digit_mission_shutdown(void);

#endif /* STN_LABZ_DIGIT_MISSION_H */