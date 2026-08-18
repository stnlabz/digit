/**
 * @file digit_policy_worker.c
 * @brief Policy worker implementation for STN-LABZ Digit Core.
 *
 * The worker provides one bounded asynchronous policy-processing job.
 *
 * The current implementation discovers Markdown document candidates in
 * the governed STN-LABZ policy directory and resolves discovered
 * candidates against a worker-owned snapshot of the recognized policy
 * index.
 *
 * Discovery and index matching do not establish authorization, approval,
 * cryptographic integrity, Trust Chain validity, or eligibility for
 * consumption.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "digit_audit.h"
#include "digit_policy_index.h"
#include "digit_policy_worker.h"
#include "platform_thread.h"

/**
 * @brief Markdown discovery search pattern.
 */
#define DIGIT_POLICY_MARKDOWN_PATTERN \
    "C:\\stn-labz\\policies\\*.md"

/**
 * @brief Maximum discovered Markdown candidates for one worker job.
 */
#define DIGIT_POLICY_DISCOVERY_MAX 256U

/**
 * @brief Maximum discovered filename length excluding null termination.
 */
#define DIGIT_POLICY_FILENAME_MAX 260U

/**
 * @brief Maximum Root Document ID derived from a candidate filename.
 */
#define DIGIT_POLICY_CANDIDATE_ROOT_ID_MAX 128U

/**
 * @brief One discovered Markdown candidate.
 *
 * Discovery records existence only.
 * It does not establish authorization.
 */
typedef struct digit_policy_discovery_record
{
    char filename[
        DIGIT_POLICY_FILENAME_MAX + 1U
    ];

    char root_document_id[
        DIGIT_POLICY_CANDIDATE_ROOT_ID_MAX + 1U
    ];

    int index_match;

} digit_policy_discovery_record_t;

/**
 * @brief Current worker state.
 */
static volatile digit_policy_worker_state_t
    g_digit_policy_worker_state =
        DIGIT_POLICY_WORKER_STOPPED;

/**
 * @brief Platform worker thread.
 */
static digit_platform_thread_t
    g_digit_policy_worker_thread = { NULL };

/**
 * @brief Indicates whether the worker component is initialized.
 */
static int g_digit_policy_worker_initialized = 0;

/**
 * @brief Worker-owned policy-index snapshot for the current job.
 */
static digit_policy_index_t
    g_digit_policy_worker_index;

/**
 * @brief Indicates whether a worker-owned index snapshot exists.
 */
static int g_digit_policy_worker_index_valid = 0;

/**
 * @brief Discovered Markdown candidates for the current worker job.
 */
static digit_policy_discovery_record_t
    g_digit_policy_discovery_records[
        DIGIT_POLICY_DISCOVERY_MAX
    ];

/**
 * @brief Number of Markdown candidates discovered by the current job.
 */
static size_t g_digit_policy_discovery_count = 0U;

/**
 * @brief Determines whether a discovered filesystem entry is usable.
 *
 * Directories and other non-file entries are rejected.
 *
 * @param data Windows filesystem discovery information.
 *
 * @return 1 when usable as a file candidate, otherwise 0.
 */
static int digit_policy_discovery_entry_is_file(
    const WIN32_FIND_DATAA *data)
{
    if (data == NULL)
    {
        return 0;
    }

    if ((data->dwFileAttributes &
         FILE_ATTRIBUTE_DIRECTORY) != 0U)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief Derives the candidate Root Document ID from a policy filename.
 *
 * STN-LABZ policy filenames begin with the Root Document ID followed by
 * either an underscore or the Markdown extension.
 *
 * Examples:
 *
 * GH-001_GITHUB_REPOSITORY_REQUIREMENTS.md -> GH-001
 * 20260730.0_DIGIT_FOUNDATION.md           -> 20260730.0
 *
 * @param filename Candidate filename.
 * @param root_document_id Destination Root Document ID buffer.
 * @param capacity Destination capacity.
 *
 * @return 0 on success, non-zero on invalid filename or capacity failure.
 */
static int digit_policy_candidate_root_id(
    const char *filename,
    char *root_document_id,
    size_t capacity)
{
    const char *separator;
    const char *extension;
    size_t length;

    if (filename == NULL ||
        root_document_id == NULL ||
        capacity == 0U)
    {
        return -1;
    }

    separator =
        strchr(
            filename,
            '_'
        );

    extension =
        strstr(
            filename,
            ".md"
        );

    if (separator != NULL)
    {
        length =
            (size_t)(separator - filename);
    }
    else if (extension != NULL &&
             extension[3] == '\0')
    {
        length =
            (size_t)(extension - filename);
    }
    else
    {
        return -1;
    }

    if (length == 0U ||
        length >= capacity)
    {
        return -1;
    }

    memcpy(
        root_document_id,
        filename,
        length
    );

    root_document_id[length] = '\0';

    return 0;
}

/**
 * @brief Searches the worker-owned index for a Root Document ID.
 *
 * This operation establishes index presence only.
 *
 * @param root_document_id Candidate Root Document ID.
 *
 * @return 1 when exactly one matching record exists, 0 when no matching
 *         record exists, and -1 when the index is ambiguous.
 */
static int digit_policy_index_find_root(
    const char *root_document_id)
{
    size_t index;
    size_t matches;

    if (root_document_id == NULL ||
        g_digit_policy_worker_index_valid == 0)
    {
        return -1;
    }

    matches = 0U;

    for (index = 0U;
         index < g_digit_policy_worker_index.record_count;
         index++)
    {
        const digit_policy_index_record_t *record;

        record =
            &g_digit_policy_worker_index.records[index];

        if (strcmp(
                record->root_document_id,
                root_document_id) == 0)
        {
            matches++;
        }
    }

    if (matches == 0U)
    {
        return 0;
    }

    if (matches == 1U)
    {
        return 1;
    }

    return -1;
}

/**
 * @brief Stores one discovered Markdown filename.
 *
 * The candidate Root Document ID is derived and its presence in the
 * worker-owned index is evaluated.
 *
 * @param filename Discovered filename.
 *
 * @return 0 on success, non-zero on invalid input, ambiguous index
 *         resolution, or capacity failure.
 */
static int digit_policy_discovery_store(
    const char *filename)
{
    size_t length;
    int match_result;

    digit_policy_discovery_record_t *record;

    if (filename == NULL)
    {
        return -1;
    }

    if (g_digit_policy_discovery_count >=
        DIGIT_POLICY_DISCOVERY_MAX)
    {
        return -1;
    }

    length = strlen(filename);

    if (length == 0U ||
        length > DIGIT_POLICY_FILENAME_MAX)
    {
        return -1;
    }

    record =
        &g_digit_policy_discovery_records[
            g_digit_policy_discovery_count
        ];

    memset(
        record,
        0,
        sizeof(*record)
    );

    memcpy(
        record->filename,
        filename,
        length + 1U
    );

    if (digit_policy_candidate_root_id(
            record->filename,
            record->root_document_id,
            sizeof(record->root_document_id)) != 0)
    {
        return -1;
    }

    match_result =
        digit_policy_index_find_root(
            record->root_document_id
        );

    /*
     * More than one index record for the same Root Document ID is
     * ambiguous at this boundary and fails the worker job.
     */
    if (match_result < 0)
    {
        return -1;
    }

    record->index_match =
        match_result;

    g_digit_policy_discovery_count++;

    return 0;
}

/**
 * @brief Discovers Markdown candidates in the governed policy directory.
 *
 * Discovery is intentionally limited to the policy directory itself.
 * Recursive traversal is not performed by this implementation.
 *
 * Document contents are not opened or parsed.
 *
 * @return 0 on successful discovery, non-zero on failure.
 */
static int digit_policy_discover_markdown(void)
{
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    DWORD error;

    memset(
        &find_data,
        0,
        sizeof(find_data)
    );

    find_handle =
        FindFirstFileA(
            DIGIT_POLICY_MARKDOWN_PATTERN,
            &find_data
        );

    if (find_handle ==
        INVALID_HANDLE_VALUE)
    {
        error = GetLastError();

        /*
         * An accessible directory containing no Markdown files is a
         * valid discovery result: zero candidates.
         */
        if (error ==
            ERROR_FILE_NOT_FOUND)
        {
            return 0;
        }

        return -1;
    }

    for (;;)
    {
        if (digit_policy_discovery_entry_is_file(
                &find_data) != 0)
        {
            if (digit_policy_discovery_store(
                    find_data.cFileName) != 0)
            {
                (void)FindClose(
                    find_handle
                );

                return -1;
            }
        }

        if (FindNextFileA(
                find_handle,
                &find_data) == 0)
        {
            error = GetLastError();

            break;
        }
    }

    if (FindClose(
            find_handle) == 0)
    {
        return -1;
    }

    if (error !=
        ERROR_NO_MORE_FILES)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Writes discovery and index-resolution results to audit evidence.
 *
 * MATCH means only that exactly one index record carries the candidate
 * Root Document ID.
 *
 * MATCH does not mean Approved, authorized, trusted, or consumable.
 *
 * @return 0 on success, non-zero on audit failure.
 */
static int digit_policy_audit_discovery(void)
{
    size_t index;

    char event[
        DIGIT_AUDIT_EVENT_MAX + 1U
    ];

    int result;

    result =
        snprintf(
            event,
            sizeof(event),
            "Policy Markdown discovery completed: %zu candidate(s).",
            g_digit_policy_discovery_count
        );

    if (result < 0 ||
        (size_t)result >= sizeof(event))
    {
        return -1;
    }

    if (digit_audit_append(
            "POLICY_WORKER",
            event) != 0)
    {
        return -1;
    }

    for (index = 0U;
         index < g_digit_policy_discovery_count;
         index++)
    {
        const digit_policy_discovery_record_t *record;

        record =
            &g_digit_policy_discovery_records[index];

        if (record->index_match != 0)
        {
            result =
                snprintf(
                    event,
                    sizeof(event),
                    "Policy candidate INDEX MATCH: %s -> %s",
                    record->filename,
                    record->root_document_id
                );
        }
        else
        {
            result =
                snprintf(
                    event,
                    sizeof(event),
                    "Policy candidate UNINDEXED: %s -> %s",
                    record->filename,
                    record->root_document_id
                );
        }

        if (result < 0 ||
            (size_t)result >= sizeof(event))
        {
            return -1;
        }

        if (digit_audit_append(
                "POLICY_WORKER",
                event) != 0)
        {
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Policy-worker thread procedure.
 *
 * The worker currently performs Markdown candidate discovery and
 * candidate-to-index Root Document ID resolution.
 *
 * Neither operation authorizes or consumes policy documents.
 *
 * @param context Reserved worker context.
 *
 * @return 0 on successful completion, otherwise non-zero.
 */
static unsigned int digit_policy_worker_thread(
    void *context)
{
    (void)context;

    if (digit_audit_append(
            "POLICY_WORKER",
            "Policy worker execution started.") != 0)
    {
        g_digit_policy_worker_state =
            DIGIT_POLICY_WORKER_FAILED;

        return 1U;
    }

    memset(
        g_digit_policy_discovery_records,
        0,
        sizeof(g_digit_policy_discovery_records)
    );

    g_digit_policy_discovery_count = 0U;

    if (digit_policy_discover_markdown() != 0)
    {
        (void)digit_audit_append(
            "POLICY_WORKER",
            "Policy Markdown discovery or index resolution failed."
        );

        g_digit_policy_worker_state =
            DIGIT_POLICY_WORKER_FAILED;

        return 1U;
    }

    if (digit_policy_audit_discovery() != 0)
    {
        g_digit_policy_worker_state =
            DIGIT_POLICY_WORKER_FAILED;

        return 1U;
    }

    if (digit_audit_append(
            "POLICY_WORKER",
            "Policy worker execution completed.") != 0)
    {
        g_digit_policy_worker_state =
            DIGIT_POLICY_WORKER_FAILED;

        return 1U;
    }

    g_digit_policy_worker_state =
        DIGIT_POLICY_WORKER_COMPLETE;

    return 0U;
}

int digit_policy_worker_init(void)
{
    if (g_digit_policy_worker_initialized != 0)
    {
        return -1;
    }

    g_digit_policy_worker_thread.handle =
        NULL;

    memset(
        &g_digit_policy_worker_index,
        0,
        sizeof(g_digit_policy_worker_index)
    );

    g_digit_policy_worker_index_valid = 0;

    memset(
        g_digit_policy_discovery_records,
        0,
        sizeof(g_digit_policy_discovery_records)
    );

    g_digit_policy_discovery_count = 0U;

    g_digit_policy_worker_state =
        DIGIT_POLICY_WORKER_IDLE;

    g_digit_policy_worker_initialized = 1;

    return 0;
}

int digit_policy_worker_start(
    const digit_policy_index_t *policy_index)
{
    if (g_digit_policy_worker_initialized == 0 ||
        policy_index == NULL)
    {
        return -1;
    }

    if (g_digit_policy_worker_state !=
        DIGIT_POLICY_WORKER_IDLE)
    {
        return -1;
    }

    /*
     * Capture the recognized index before asynchronous execution begins.
     *
     * The worker therefore does not depend upon mutable caller-owned
     * index storage while the thread is running.
     */
    memcpy(
        &g_digit_policy_worker_index,
        policy_index,
        sizeof(g_digit_policy_worker_index)
    );

    g_digit_policy_worker_index_valid = 1;

    g_digit_policy_worker_state =
        DIGIT_POLICY_WORKER_RUNNING;

    if (digit_platform_thread_create(
            &g_digit_policy_worker_thread,
            digit_policy_worker_thread,
            NULL) != 0)
    {
        memset(
            &g_digit_policy_worker_index,
            0,
            sizeof(g_digit_policy_worker_index)
        );

        g_digit_policy_worker_index_valid = 0;

        g_digit_policy_worker_state =
            DIGIT_POLICY_WORKER_FAILED;

        (void)digit_audit_append(
            "POLICY_WORKER",
            "Policy worker thread creation failed."
        );

        return -1;
    }

    return 0;
}

digit_policy_worker_state_t
digit_policy_worker_get_state(void)
{
    return g_digit_policy_worker_state;
}

int digit_policy_worker_reset(void)
{
    if (g_digit_policy_worker_initialized == 0)
    {
        return -1;
    }

    if (g_digit_policy_worker_state !=
            DIGIT_POLICY_WORKER_COMPLETE &&
        g_digit_policy_worker_state !=
            DIGIT_POLICY_WORKER_FAILED)
    {
        return -1;
    }

    if (g_digit_policy_worker_thread.handle != NULL)
    {
        if (digit_platform_thread_join(
                &g_digit_policy_worker_thread) != 0)
        {
            return -1;
        }

        digit_platform_thread_close(
            &g_digit_policy_worker_thread
        );
    }

    memset(
        &g_digit_policy_worker_index,
        0,
        sizeof(g_digit_policy_worker_index)
    );

    g_digit_policy_worker_index_valid = 0;

    memset(
        g_digit_policy_discovery_records,
        0,
        sizeof(g_digit_policy_discovery_records)
    );

    g_digit_policy_discovery_count = 0U;

    g_digit_policy_worker_state =
        DIGIT_POLICY_WORKER_IDLE;

    return 0;
}

int digit_policy_worker_is_ready(void)
{
    return g_digit_policy_worker_initialized;
}

void digit_policy_worker_shutdown(void)
{
    if (g_digit_policy_worker_initialized == 0)
    {
        return;
    }

    /*
     * Do not abandon a running worker during controlled Core shutdown.
     */
    if (g_digit_policy_worker_thread.handle != NULL)
    {
        (void)digit_platform_thread_join(
            &g_digit_policy_worker_thread
        );

        digit_platform_thread_close(
            &g_digit_policy_worker_thread
        );
    }

    memset(
        &g_digit_policy_worker_index,
        0,
        sizeof(g_digit_policy_worker_index)
    );

    g_digit_policy_worker_index_valid = 0;

    memset(
        g_digit_policy_discovery_records,
        0,
        sizeof(g_digit_policy_discovery_records)
    );

    g_digit_policy_discovery_count = 0U;

    g_digit_policy_worker_state =
        DIGIT_POLICY_WORKER_STOPPED;

    g_digit_policy_worker_initialized = 0;
}