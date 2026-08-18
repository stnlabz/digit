/**
 * @file digit_audit.c
 * @brief Audit and evidence implementation for STN-LABZ Digit Core.
 *
 * Audit evidence is maintained in bounded runtime memory and persisted
 * to the governed Digit audit log.
 *
 * Persistent logging uses append mode. Existing evidence is never
 * intentionally truncated or overwritten.
 *
 * The persistent audit file is opened only for the duration of each
 * individual append operation so operators may inspect the log while
 * Digit remains operational.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "digit_audit.h"

 /**
  * @brief Core-owned runtime audit ledger.
  */
static digit_audit_record_t g_digit_audit_records[
    DIGIT_AUDIT_MAX_RECORDS
];

/**
 * @brief Number of active runtime records.
 */
static size_t g_digit_audit_count = 0U;

/**
 * @brief Next runtime sequence number.
 */
static unsigned long g_digit_audit_next_sequence = 1UL;

/**
 * @brief Indicates whether audit is initialized.
 */
static int g_digit_audit_initialized = 0;

/**
 * @brief Copies bounded text into audit storage.
 *
 * @param destination Destination buffer.
 * @param capacity Destination capacity.
 * @param source Source text.
 *
 * @return 0 on success, non-zero on invalid or oversized input.
 */
static int digit_audit_copy_text(
    char* destination,
    size_t capacity,
    const char* source)
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

/**
 * @brief Verifies that the governed persistent audit log can be opened.
 *
 * The file is opened in append mode and immediately closed.
 *
 * This proves that the logging boundary is available without retaining
 * a process-lifetime file handle.
 *
 * @return 0 on success, non-zero on failure.
 */
static int digit_audit_verify_persistent_log(void)
{
    FILE* audit_file;

    audit_file = NULL;

    if (fopen_s(
        &audit_file,
        DIGIT_AUDIT_LOG_PATH,
        "a") != 0 ||
        audit_file == NULL)
    {
        return -1;
    }

    if (fclose(audit_file) != 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Writes one record to the persistent audit log.
 *
 * The persistent file is opened in append mode, written, flushed,
 * and immediately closed.
 *
 * @param record Record to persist.
 *
 * @return 0 on success, non-zero on failure.
 */
static int digit_audit_persist_record(
    const digit_audit_record_t* record)
{
    FILE* audit_file;
    int write_result;

    if (record == NULL)
    {
        return -1;
    }

    audit_file = NULL;

    /*
     * Append mode preserves all existing evidence.
     */
    if (fopen_s(
        &audit_file,
        DIGIT_AUDIT_LOG_PATH,
        "a") != 0 ||
        audit_file == NULL)
    {
        return -1;
    }

    write_result =
        fprintf(
            audit_file,
            "%lu | %s | %s\n",
            record->sequence,
            record->component,
            record->event
        );

    if (write_result < 0)
    {
        (void)fclose(
            audit_file
        );

        return -1;
    }

    /*
     * Force the accepted record through the CRT buffer before closing.
     */
    if (fflush(audit_file) != 0)
    {
        (void)fclose(
            audit_file
        );

        return -1;
    }

    /*
     * The file handle is released after every accepted append so the
     * operator may inspect persistent evidence while Digit is running.
     */
    if (fclose(audit_file) != 0)
    {
        return -1;
    }

    return 0;
}

int digit_audit_init(void)
{
    if (g_digit_audit_initialized != 0)
    {
        return -1;
    }

    memset(
        g_digit_audit_records,
        0,
        sizeof(g_digit_audit_records)
    );

    g_digit_audit_count = 0U;
    g_digit_audit_next_sequence = 1UL;

    /*
     * Establish that the governed persistent logging boundary is
     * available.
     *
     * The verification handle is immediately released.
     */
    if (digit_audit_verify_persistent_log() != 0)
    {
        return -1;
    }

    g_digit_audit_initialized = 1;

    return 0;
}

int digit_audit_append(
    const char* component,
    const char* event)
{
    digit_audit_record_t pending_record;
    digit_audit_record_t* record;

    if (g_digit_audit_initialized == 0 ||
        component == NULL ||
        event == NULL)
    {
        return -1;
    }

    /*
     * Runtime evidence remains bounded and append-only.
     */
    if (g_digit_audit_count >=
        DIGIT_AUDIT_MAX_RECORDS)
    {
        return -1;
    }

    memset(
        &pending_record,
        0,
        sizeof(pending_record)
    );

    if (digit_audit_copy_text(
        pending_record.component,
        sizeof(pending_record.component),
        component) != 0)
    {
        return -1;
    }

    if (digit_audit_copy_text(
        pending_record.event,
        sizeof(pending_record.event),
        event) != 0)
    {
        return -1;
    }

    pending_record.sequence =
        g_digit_audit_next_sequence;

    /*
     * Persistent evidence is established before the corresponding
     * runtime record is committed.
     */
    if (digit_audit_persist_record(
        &pending_record) != 0)
    {
        return -1;
    }

    /*
     * Persistent evidence exists.
     * Commit the corresponding runtime representation.
     */
    record =
        &g_digit_audit_records[
            g_digit_audit_count
        ];

    memcpy(
        record,
        &pending_record,
        sizeof(*record)
    );

    g_digit_audit_count++;
    g_digit_audit_next_sequence++;

    return 0;
}

size_t digit_audit_count(void)
{
    if (g_digit_audit_initialized == 0)
    {
        return 0U;
    }

    return g_digit_audit_count;
}

const digit_audit_record_t* digit_audit_get(
    size_t index)
{
    if (g_digit_audit_initialized == 0)
    {
        return NULL;
    }

    if (index >= g_digit_audit_count)
    {
        return NULL;
    }

    return &g_digit_audit_records[index];
}

int digit_audit_is_ready(void)
{
    return g_digit_audit_initialized;
}

void digit_audit_shutdown(void)
{
    /*
     * No persistent file handle is retained during runtime.
     *
     * Persistent evidence already written to disk remains untouched.
     */
    memset(
        g_digit_audit_records,
        0,
        sizeof(g_digit_audit_records)
    );

    g_digit_audit_count = 0U;
    g_digit_audit_next_sequence = 1UL;

    g_digit_audit_initialized = 0;
}