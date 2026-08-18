/**
 * @file digit_core.c
 * @brief Lifecycle implementation for the STN-LABZ Digit Core.
 *
 * Digit Core owns the system safety nucleus and controls initialization
 * and shutdown of Core components.
 *
 * Current initialized Core safety-nucleus components:
 *
 * - Core identity;
 * - General Orders;
 * - human-authority boundary;
 * - runtime mission state;
 * - result / epistemic states;
 * - audit / evidence;
 * - Safe Mode;
 * - Company Preservation;
 * - operator reporting;
 * - deterministic configuration.
 */

#include "digit_core.h"
#include "digit_audit.h"
#include "digit_authority.h"
#include "digit_company_preservation.h"
#include "digit_config.h"
#include "digit_general_orders.h"
#include "digit_identity.h"
#include "digit_mission.h"
#include "digit_operator_report.h"
#include "digit_result.h"
#include "digit_safe_mode.h"

/**
 * @brief Current Core lifecycle state.
 */
static digit_core_state_t g_digit_core_state =
    DIGIT_CORE_UNINITIALIZED;

/**
 * @brief Core configuration instance.
 */
static digit_config_t g_core_config;

int digit_core_init(void)
{
    if (g_digit_core_state !=
            DIGIT_CORE_UNINITIALIZED &&
        g_digit_core_state !=
            DIGIT_CORE_STOPPED)
    {
        return -1;
    }

    g_digit_core_state =
        DIGIT_CORE_INITIALIZING;

    if (digit_identity_init() != 0)
    {
        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_general_orders_init() != 0)
    {
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_authority_init() != 0)
    {
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_mission_init() != 0)
    {
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_result_init() != 0)
    {
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_audit_init() != 0)
    {
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_audit_append(
            "CORE",
            "Runtime audit boundary initialized.") != 0)
    {
        digit_audit_shutdown();
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_safe_mode_init() != 0)
    {
        digit_audit_shutdown();
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_company_preservation_init() != 0)
    {
        digit_safe_mode_shutdown();
        digit_audit_shutdown();
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Operator reporting becomes available before configuration and before
     * Core declares READY.
     */
    if (digit_operator_report_init() != 0)
    {
        digit_company_preservation_shutdown();
        digit_safe_mode_shutdown();
        digit_audit_shutdown();
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_config_init(
            &g_core_config) != 0)
    {
        digit_operator_report_shutdown();
        digit_company_preservation_shutdown();
        digit_safe_mode_shutdown();
        digit_audit_shutdown();
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    if (digit_audit_append(
            "CORE",
            "Core initialization boundaries satisfied.") != 0)
    {
        digit_config_shutdown();
        digit_operator_report_shutdown();
        digit_company_preservation_shutdown();
        digit_safe_mode_shutdown();
        digit_audit_shutdown();
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Emit initial operator-visible readiness report.
     */
    if (digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_INFO,
            "CORE",
            "Digit Core initialization complete.") != 0)
    {
        digit_config_shutdown();
        digit_operator_report_shutdown();
        digit_company_preservation_shutdown();
        digit_safe_mode_shutdown();
        digit_audit_shutdown();
        digit_result_shutdown();
        digit_mission_shutdown();
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    g_digit_core_state =
        DIGIT_CORE_READY;

    return 0;
}

digit_core_state_t digit_core_get_state(void)
{
    return g_digit_core_state;
}

void digit_core_shutdown(void)
{
    if (g_digit_core_state ==
            DIGIT_CORE_UNINITIALIZED ||
        g_digit_core_state ==
            DIGIT_CORE_STOPPED)
    {
        return;
    }

    g_digit_core_state =
        DIGIT_CORE_SHUTTING_DOWN;

    /*
     * Produce operator and audit evidence before reporting and audit
     * boundaries are withdrawn.
     */
    (void)digit_operator_report_emit(
        DIGIT_OPERATOR_REPORT_NOTICE,
        "CORE",
        "Controlled Core shutdown initiated."
    );

    (void)digit_audit_append(
        "CORE",
        "Controlled Core shutdown initiated."
    );

    /*
     * Shutdown occurs in reverse initialization order.
     */
    digit_config_shutdown();
    digit_operator_report_shutdown();
    digit_company_preservation_shutdown();
    digit_safe_mode_shutdown();
    digit_audit_shutdown();
    digit_result_shutdown();
    digit_mission_shutdown();
    digit_authority_shutdown();
    digit_general_orders_shutdown();
    digit_identity_shutdown();

    g_digit_core_state =
        DIGIT_CORE_STOPPED;
}