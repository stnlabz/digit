/**
 * @file digit_mission_control.h
 * @brief Mission-control integration for STN-LABZ Digit Core.
 *
 * This component connects the human-authority boundary, runtime mission
 * state, fail-safe states, audit evidence, and operator reporting.
 *
 * The operator interface may submit a mission, but Core determines whether
 * that mission may be accepted and activated.
 */

#ifndef STN_LABZ_DIGIT_MISSION_CONTROL_H
#define STN_LABZ_DIGIT_MISSION_CONTROL_H

/**
 * @brief Assigns and activates an operator-provided mission.
 *
 * Mission acceptance requires:
 *
 * - Core mission-control component active;
 * - explicit human authority established;
 * - Safe Mode inactive;
 * - Company Preservation inactive;
 * - valid mission assignment;
 * - valid mission activation.
 *
 * @param mission_text Non-empty mission description.
 * @param human_authority_established Non-zero only when the operator
 *        interface has established human authority.
 *
 * @return 0 on success, non-zero on rejection or failure.
 */
int digit_mission_control_assign(
    const char *mission_text,
    int human_authority_established
);

/**
 * @brief Initializes mission-control integration.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_mission_control_init(void);

/**
 * @brief Determines whether mission-control integration is active.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_mission_control_is_ready(void);

/**
 * @brief Withdraws mission-control integration during Core shutdown.
 */
void digit_mission_control_shutdown(void);

#endif /* STN_LABZ_DIGIT_MISSION_CONTROL_H */