/**
 * @file digit_core.c
 * @brief Lifecycle implementation for the STN-LABZ Digit Core.
 *
 * Digit Core owns the system safety nucleus and controls initialization
 * and shutdown of Core components.
 *
 * Current initialized Core components:
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
 * - mission-control integration;
 * - policy worker;
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
#include "digit_mission_control.h"
#include "digit_operator_report.h"
#include "digit_policy_worker.h"
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
    /*
     * Core may initialize only from an uninitialized or previously
     * stopped state.
     */
    if (g_digit_core_state !=
            DIGIT_CORE_UNINITIALIZED &&
        g_digit_core_state !=
            DIGIT_CORE_STOPPED)
    {
        return -1;
    }

    g_digit_core_state =
        DIGIT_CORE_INITIALIZING;

    /*
     * Establish immutable Digit identity.
     */
    if (digit_identity_init() != 0)
    {
        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Activate standing General Orders.
     */
    if (digit_general_orders_init() != 0)
    {
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Establish the human-authority boundary.
     */
    if (digit_authority_init() != 0)
    {
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Establish deterministic runtime mission state.
     *
     * Digit begins with no assigned mission.
     */
    if (digit_mission_init() != 0)
    {
        digit_authority_shutdown();
        digit_general_orders_shutdown();
        digit_identity_shutdown();

        g_digit_core_state =
            DIGIT_CORE_STOPPED;

        return -1;
    }

    /*
     * Establish deterministic result and epistemic states.
     */
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

    /*
     * Establish persistent Core audit and evidence logging.
     *
     * If required logging cannot be established, Core does not
     * proceed to READY.
     */
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

    /*
     * Establish Safe Mode.
     *
     * Safe Mode begins inactive but must exist before normal
     * operational capability becomes available.
     */
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

    /*
     * Establish Company Preservation.
     *
     * This remains separate from ordinary mission state and Safe Mode.
     */
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
     * Establish structured operator reporting.
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

    /*
     * Establish Core mission-control integration.
     *
     * Mission control depends upon:
     *
     * - authority;
     * - mission state;
     * - audit;
     * - Safe Mode;
     * - Company Preservation;
     * - operator reporting.
     *
     * Those boundaries therefore initialize first.
     */
    if (digit_mission_control_init() != 0)
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

    /*
     * Establish the Core-owned policy worker.
     *
     * Initialization creates an IDLE worker only. No policy-processing
     * job is started during Core initialization.
     */
    if (digit_policy_worker_init() != 0)
    {
        digit_mission_control_shutdown();
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
     * Initialize deterministic Core configuration.
     */
    if (digit_config_init(
            &g_core_config) != 0)
    {
        digit_policy_worker_shutdown();
        digit_mission_control_shutdown();
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
     * All current Core initialization boundaries have succeeded.
     */
    if (digit_audit_append(
            "CORE",
            "Core initialization boundaries satisfied.") != 0)
    {
        digit_config_shutdown();
        digit_policy_worker_shutdown();
        digit_mission_control_shutdown();
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
     * Produce operator-visible readiness evidence before Core declares
     * itself READY.
     */
    if (digit_operator_report_emit(
            DIGIT_OPERATOR_REPORT_INFO,
            "CORE",
            "Digit Core initialization complete.") != 0)
    {
        digit_config_shutdown();
        digit_policy_worker_shutdown();
        digit_mission_control_shutdown();
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
     * Core reaches READY only after all currently required boundaries
     * have initialized successfully.
     */
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
    /*
     * Shutdown is unnecessary when Core has never initialized or has
     * already stopped.
     */
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
     * Produce operator and audit evidence before those boundaries
     * are withdrawn.
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
     * Shutdown occurs in reverse dependency order.
     */
    digit_config_shutdown();
    digit_policy_worker_shutdown();
    digit_mission_control_shutdown();
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