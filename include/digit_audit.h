/**
 * @file digit_audit.h
 * @brief Audit and evidence interface for STN-LABZ Digit Core.
 *
 * Audit and evidence are part of Digit's Core safety nucleus.
 *
 * The initial implementation provides bounded, append-only, Core-owned
 * runtime evidence.
 *
 * Evidence is not silently overwritten or deleted during operation.
 */

#ifndef STN_LABZ_DIGIT_AUDIT_H
#define STN_LABZ_DIGIT_AUDIT_H

#include <stddef.h>

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
 * @brief Initializes Core-owned audit state.
 *
 * The ledger begins empty.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_audit_init(void);

/**
 * @brief Appends one evidence record.
 *
 * Existing records are never overwritten.
 *
 * The operation fails when:
 *
 * - the audit component is inactive;
 * - component or event is NULL;
 * - component or event is empty;
 * - either value exceeds its bounded length;
 * - the ledger has reached capacity.
 *
 * @param component Component producing the evidence.
 * @param event Deterministic event description.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_audit_append(
    const char *component,
    const char *event
);

/**
 * @brief Returns the current number of evidence records.
 *
 * @return Record count, or 0 when audit is not initialized.
 */
size_t digit_audit_count(void);

/**
 * @brief Returns one audit record by zero-based index.
 *
 * @param index Record index.
 *
 * @return Read-only record pointer, or NULL when unavailable.
 */
const digit_audit_record_t *digit_audit_get(
    size_t index
);

/**
 * @brief Determines whether the audit component is active.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_audit_is_ready(void);

/**
 * @brief Clears runtime audit storage during controlled Core shutdown.
 *
 * Persistent audit preservation is outside this initial runtime component
 * and will be implemented separately when the applicable evidence-storage
 * boundary is developed.
 */
void digit_audit_shutdown(void);

#endif /* STN_LABZ_DIGIT_AUDIT_H */