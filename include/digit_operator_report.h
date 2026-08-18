/**
 * @file digit_operator_report.h
 * @brief Operator reporting interface for STN-LABZ Digit Core.
 *
 * Operator reporting is part of Digit's Core safety nucleus.
 *
 * Core produces deterministic structured reports for presentation to an
 * authorized operator interface.
 *
 * This component does not perform console, IRC, GUI, or network output.
 */

#ifndef STN_LABZ_DIGIT_OPERATOR_REPORT_H
#define STN_LABZ_DIGIT_OPERATOR_REPORT_H

#include <stddef.h>

/**
 * @brief Maximum number of retained runtime operator reports.
 */
#define DIGIT_OPERATOR_REPORT_MAX_RECORDS 128U

/**
 * @brief Maximum report component length excluding null termination.
 */
#define DIGIT_OPERATOR_REPORT_COMPONENT_MAX 64U

/**
 * @brief Maximum report message length excluding null termination.
 */
#define DIGIT_OPERATOR_REPORT_MESSAGE_MAX 256U

/**
 * @brief Operator report classification.
 */
typedef enum digit_operator_report_type
{
    DIGIT_OPERATOR_REPORT_INFO = 0,
    DIGIT_OPERATOR_REPORT_NOTICE,
    DIGIT_OPERATOR_REPORT_WARNING,
    DIGIT_OPERATOR_REPORT_FAILURE,
    DIGIT_OPERATOR_REPORT_AUTHORITY_REQUIRED,
    DIGIT_OPERATOR_REPORT_SAFE_MODE,
    DIGIT_OPERATOR_REPORT_COMPANY_PRESERVATION
} digit_operator_report_type_t;

/**
 * @brief One structured operator report.
 */
typedef struct digit_operator_report
{
    unsigned long sequence;

    digit_operator_report_type_t type;

    char component[
        DIGIT_OPERATOR_REPORT_COMPONENT_MAX + 1U
    ];

    char message[
        DIGIT_OPERATOR_REPORT_MESSAGE_MAX + 1U
    ];

} digit_operator_report_t;

/**
 * @brief Initializes Core-owned operator reporting.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_operator_report_init(void);

/**
 * @brief Emits one structured operator report.
 *
 * Reports are retained in append-only runtime order.
 *
 * Existing reports are not overwritten when capacity is reached.
 *
 * @param type Report classification.
 * @param component Reporting Core component.
 * @param message Operator-facing report message.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_operator_report_emit(
    digit_operator_report_type_t type,
    const char *component,
    const char *message
);

/**
 * @brief Returns the current number of retained reports.
 *
 * @return Report count, or 0 when reporting is inactive.
 */
size_t digit_operator_report_count(void);

/**
 * @brief Returns one report by zero-based index.
 *
 * @param index Report index.
 *
 * @return Read-only report pointer, or NULL when unavailable.
 */
const digit_operator_report_t *digit_operator_report_get(
    size_t index
);

/**
 * @brief Returns the most recent operator report.
 *
 * @return Read-only report pointer, or NULL when no report exists.
 */
const digit_operator_report_t *digit_operator_report_latest(void);

/**
 * @brief Determines whether operator reporting is active.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_operator_report_is_ready(void);

/**
 * @brief Clears runtime operator-report state during controlled shutdown.
 */
void digit_operator_report_shutdown(void);

#endif /* STN_LABZ_DIGIT_OPERATOR_REPORT_H */