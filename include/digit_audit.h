/**
 * @file digit_audit.h
 * @brief Audit and evidence interface for STN-LABZ Digit Core.
 *
 * Audit and evidence are part of Digit's Core safety nucleus.
 *
 * Audit evidence is retained in bounded runtime memory and persisted to
 * the governed Digit log location.
 *
 * Persistent evidence is append-only. Existing evidence is never
 * intentionally truncated or overwritten by this component.
 */

#ifndef STN_LABZ_DIGIT_AUDIT_H
#define STN_LABZ_DIGIT_AUDIT_H

#include <stddef.h>

 /**
  * @brief Governed Digit audit-log location.
  */
#define DIGIT_AUDIT_LOG_PATH \
    "C:\\stn-labz\\digit\\logs\\digit-audit.log"

  /**
   * @brief Maximum number of runtime audit records.
   */
#define DIGIT_AUDIT_MAX_RECORDS 256U

   /**
    * @brief Maximum component-name length excluding null termination.
    */
#define DIGIT_AUDIT_COMPONENT_MAX 64U

    /**
     * @brief Maximum event-text length excluding null termination.
     */
#define DIGIT_AUDIT_EVENT_MAX 256U

     /**
      * @brief One immutable runtime audit record.
      */
typedef struct digit_audit_record
{
    unsigned long sequence;

    char component[
        DIGIT_AUDIT_COMPONENT_MAX + 1U
    ];

    char event[
        DIGIT_AUDIT_EVENT_MAX + 1U
    ];

} digit_audit_record_t;

/**
 * @brief Initializes Core-owned audit state and persistent logging.
 *
 * The persistent log is opened in append mode.
 *
 * Initialization fails if the governed audit log cannot be opened.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_audit_init(void);

/**
 * @brief Appends one evidence record.
 *
 * A record is accepted only when it can be written and flushed to the
 * persistent audit log.
 *
 * Existing persistent evidence is never intentionally overwritten.
 *
 * @param component Component producing the evidence.
 * @param event Deterministic event description.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_audit_append(
    const char* component,
    const char* event
);

/**
 * @brief Returns the current number of runtime evidence records.
 *
 * @return Record count, or 0 when audit is not initialized.
 */
size_t digit_audit_count(void);

/**
 * @brief Returns one runtime audit record by zero-based index.
 *
 * @param index Record index.
 *
 * @return Read-only record pointer, or NULL when unavailable.
 */
const digit_audit_record_t* digit_audit_get(
    size_t index
);

/**
 * @brief Determines whether the audit component is active.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_audit_is_ready(void);

/**
 * @brief Closes persistent audit logging and clears runtime storage.
 *
 * Persistent log contents are not deleted or truncated.
 */
void digit_audit_shutdown(void);

#endif /* STN_LABZ_DIGIT_AUDIT_H */