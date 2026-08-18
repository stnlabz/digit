/**
 * @file digit_operator_report.c
 * @brief Operator reporting implementation for STN-LABZ Digit Core.
 *
 * Core produces structured operator reports independently of the transport
 * or presentation mechanism used to display them.
 *
 * Reports are retained in bounded append-only runtime order.
 */

#include <stddef.h>
#include <string.h>

#include "digit_audit.h"
#include "digit_operator_report.h"

/**
 * @brief Core-owned runtime operator-report storage.
 */
static digit_operator_report_t g_digit_operator_reports[
    DIGIT_OPERATOR_REPORT_MAX_RECORDS
];

/**
 * @brief Number of retained reports.
 */
static size_t g_digit_operator_report_count = 0U;

/**
 * @brief Next report sequence.
 */
static unsigned long g_digit_operator_report_next_sequence = 1UL;

/**
 * @brief Indicates whether operator reporting is initialized.
 */
static int g_digit_operator_report_initialized = 0;

/**
 * @brief Copies bounded report text.
 *
 * @param destination Destination buffer.
 * @param capacity Destination capacity.
 * @param source Source text.
 *
 * @return 0 on success, non-zero on invalid or oversized input.
 */
static int digit_operator_report_copy(
    char *destination,
    size_t capacity,
    const char *source)
{
    size_t length;

    if (destination == NULL ||
        source == NULL ||
        capacity == 0U)
    {
        return -1;
    }

    length = strlen(source);

    if (length == 0U ||
        length >= capacity)
    {
        return -1;
    }

    memcpy(
        destination,
        source,
        length + 1U
    );

    return 0;
}

int digit_operator_report_init(void)
{
    if (g_digit_operator_report_initialized != 0)
    {
        return -1;
    }

    memset(
        g_digit_operator_reports,
        0,
        sizeof(g_digit_operator_reports)
    );

    g_digit_operator_report_count = 0U;
    g_digit_operator_report_next_sequence = 1UL;

    g_digit_operator_report_initialized = 1;

    return 0;
}

int digit_operator_report_emit(
    digit_operator_report_type_t type,
    const char *component,
    const char *message)
{
    digit_operator_report_t *report;

    if (g_digit_operator_report_initialized == 0 ||
        component == NULL ||
        message == NULL)
    {
        return -1;
    }

    if (type < DIGIT_OPERATOR_REPORT_INFO ||
        type > DIGIT_OPERATOR_REPORT_COMPANY_PRESERVATION)
    {
        return -1;
    }

    /*
     * Reports are append-only.
     *
     * Capacity exhaustion does not overwrite older reports.
     */
    if (g_digit_operator_report_count >=
        DIGIT_OPERATOR_REPORT_MAX_RECORDS)
    {
        return -1;
    }

    report =
        &g_digit_operator_reports[
            g_digit_operator_report_count
        ];

    memset(
        report,
        0,
        sizeof(*report)
    );

    if (digit_operator_report_copy(
            report->component,
            sizeof(report->component),
            component) != 0)
    {
        return -1;
    }

    if (digit_operator_report_copy(
            report->message,
            sizeof(report->message),
            message) != 0)
    {
        memset(
            report,
            0,
            sizeof(*report)
        );

        return -1;
    }

    report->sequence =
        g_digit_operator_report_next_sequence;

    report->type = type;

    g_digit_operator_report_next_sequence++;
    g_digit_operator_report_count++;

    /*
     * Operator reporting also leaves an audit indication.
     *
     * Failure of the audit append does not destroy the already-created
     * operator report.
     */
    (void)digit_audit_append(
        "OPERATOR_REPORT",
        "Operator report emitted."
    );

    return 0;
}

size_t digit_operator_report_count(void)
{
    if (g_digit_operator_report_initialized == 0)
    {
        return 0U;
    }

    return g_digit_operator_report_count;
}

const digit_operator_report_t *digit_operator_report_get(
    size_t index)
{
    if (g_digit_operator_report_initialized == 0)
    {
        return NULL;
    }

    if (index >= g_digit_operator_report_count)
    {
        return NULL;
    }

    return &g_digit_operator_reports[index];
}

const digit_operator_report_t *digit_operator_report_latest(void)
{
    if (g_digit_operator_report_initialized == 0 ||
        g_digit_operator_report_count == 0U)
    {
        return NULL;
    }

    return &g_digit_operator_reports[
        g_digit_operator_report_count - 1U
    ];
}

int digit_operator_report_is_ready(void)
{
    return g_digit_operator_report_initialized;
}

void digit_operator_report_shutdown(void)
{
    memset(
        g_digit_operator_reports,
        0,
        sizeof(g_digit_operator_reports)
    );

    g_digit_operator_report_count = 0U;
    g_digit_operator_report_next_sequence = 1UL;

    g_digit_operator_report_initialized = 0;
}