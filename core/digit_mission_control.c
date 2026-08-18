/**
 * @file digit_mission_control.c
 * @brief Mission-control integration for STN-LABZ Digit Core.
 *
 * Mission-control integration enforces Core boundaries before an
 * operator-provided mission may become active.
 */

#include "digit_audit.h"
#include "digit_authority.h"
#include "digit_company_preservation.h"
#include "digit_mission.h"
#include "digit_mission_control.h"
#include "digit_operator_report.h"
#include "digit_safe_mode.h"

/**
 * @brief Indicates whether mission-control integration is active.
 */
static int g_digit_mission_control_initialized = 0;

int digit_mission_control_init(void)
{
    if (g_digit_mission_control_initialized != 0)
    {
        return -1;
    }

    g_digit_mission_control_initialized = 1;

    return 0;
}

int digit_mission_control_assign(
    const char *mission_text,
    int human_authority_established)
{
    digit_authority_result_t authority;

    if (g_digit_mission_control_initialized == 0 ||
        mission_text == NULL)
    {
        return -1;
    }

    /*
     * Fail-safe states block ordinary mission assignment.
     */
    if (digit_safe_mode_is_active() != 0)
    {
        (void)digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_SAFE_MODE,
            "MISSION",
            "Mission assignment blocked while Safe Mode is active."
        );

        return -1;
    }

    if (digit_company_preservation_is_active() != 0)
    {
        (void)digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_COMPANY_PRESERVATION,
            "MISSION",
            "Mission assignment blocked during Company Preservation."
        );

        return -1;
    }

    /*
     * Mission assignment requires established human authority.
     */
    if (digit_authority_evaluate(
            1,
            human_authority_established,
            &authority) != 0)
    {
        return -1;
    }

    if (authority.state !=
        DIGIT_AUTHORITY_GRANTED)
    {
        (void)digit_audit_append(
            "MISSION",
            "Mission assignment rejected: human authority not established."
        );

        (void)digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_AUTHORITY_REQUIRED,
            "MISSION",
            "Mission assignment requires established human authority."
        );

        return -1;
    }

    /*
     * Preserve evidence before changing mission state.
     */
    if (digit_audit_append(
            "MISSION",
            "Authorized mission assignment received.") != 0)
    {
        return -1;
    }

    if (digit_mission_assign(
            mission_text) != 0)
    {
        (void)digit_audit_append(
            "MISSION",
            "Mission assignment rejected by mission state machine."
        );

        (void)digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_FAILURE,
            "MISSION",
            "Mission assignment rejected."
        );

        return -1;
    }

    if (digit_mission_activate() != 0)
    {
        (void)digit_audit_append(
            "MISSION",
            "Mission activation failed."
        );

        /*
         * Mission state is no longer cleanly operational.
         * Preserve fail-safe behavior.
         */
        (void)digit_safe_mode_enter(
            "Mission activation failed after assignment."
        );

        (void)digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_FAILURE,
            "MISSION",
            "Mission activation failed. Safe Mode entered."
        );

        return -1;
    }

    if (digit_audit_append(
            "MISSION",
            "Mission assigned and activated.") != 0)
    {
        /*
         * Required evidence could not be preserved after state change.
         * Do not continue ordinary operation.
         */
        (void)digit_safe_mode_enter(
            "Mission audit evidence could not be preserved."
        );

        (void)digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_FAILURE,
            "MISSION",
            "Mission evidence failure. Safe Mode entered."
        );

        return -1;
    }

    (void)digit_operator_report_emit(
        DIGIT_OPERATOR_REPORT_NOTICE,
        "MISSION",
        "Mission accepted and active."
    );

    return 0;
}

int digit_mission_control_is_ready(void)
{
    return g_digit_mission_control_initialized;
}

void digit_mission_control_shutdown(void)
{
    g_digit_mission_control_initialized = 0;
}