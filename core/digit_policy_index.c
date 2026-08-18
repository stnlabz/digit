/**
 * @file digit_policy_index.c
 * @brief Machine-readable policy index implementation for STN-LABZ Digit.
 *
 * This component implements the first Digit Core JSON capability required
 * to consume the established STN-LABZ machine-readable policy index.
 *
 * Current scope:
 *
 * - bounded JSON file reading;
 * - top-level array recognition;
 * - object recognition;
 * - required string-field extraction;
 * - duplicate-field rejection;
 * - unknown-field rejection;
 * - required-field enforcement;
 * - SHA-256 hexadecimal syntax validation.
 *
 * This is intentionally not yet a complete general-purpose JSON parser.
 *
 * Successful recognition does not establish authorization, integrity,
 * Trust Chain validity, or operational acceptance.
 */

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "digit_policy_index.h"
#include "platform_fs.h"

/**
 * @brief Core-owned policy index.
 */
static digit_policy_index_t g_policy_index;

/**
 * @brief Core-owned structured-document read buffer.
 *
 * The buffer is static intentionally. The STN-LABZ structured-document
 * boundary permits documents up to exactly 8 MiB, which must not be placed
 * on the Windows thread stack.
 *
 * One additional byte is reserved for null termination.
 */
static char g_policy_index_buffer[
    DIGIT_STRUCTURED_DOCUMENT_MAX + 1U
];

/**
 * @brief Indicates whether policy index storage is initialized.
 */
static int g_policy_index_initialized = 0;

/**
 * @brief JSON parser cursor.
 */
typedef struct digit_json_cursor
{
    const char *current;

} digit_json_cursor_t;

/**
 * @brief Required-field presence flags for one index record.
 */
typedef struct digit_policy_index_fields
{
    int root_document_id;
    int revision_id;
    int title;
    int status;
    int previous_revision;
    int sha256;

} digit_policy_index_fields_t;

/**
 * @brief Skips JSON whitespace.
 *
 * @param cursor Parser cursor.
 */
static void digit_json_skip_whitespace(
    digit_json_cursor_t *cursor)
{
    if (cursor == NULL)
    {
        return;
    }

    while (*cursor->current == ' ' ||
           *cursor->current == '\t' ||
           *cursor->current == '\r' ||
           *cursor->current == '\n')
    {
        cursor->current++;
    }
}

/**
 * @brief Converts one JSON escape sequence to its character value.
 *
 * Unicode escape processing is intentionally outside the current parser
 * stage. UTF-8 content stored directly in JSON strings is preserved.
 *
 * @param escaped Escape character following a backslash.
 * @param output Receives decoded character.
 *
 * @return 0 on supported escape, non-zero otherwise.
 */
static int digit_json_decode_escape(
    char escaped,
    char *output)
{
    if (output == NULL)
    {
        return -1;
    }

    switch (escaped)
    {
        case '"':
            *output = '"';
            return 0;

        case '\\':
            *output = '\\';
            return 0;

        case '/':
            *output = '/';
            return 0;

        case 'b':
            *output = '\b';
            return 0;

        case 'f':
            *output = '\f';
            return 0;

        case 'n':
            *output = '\n';
            return 0;

        case 'r':
            *output = '\r';
            return 0;

        case 't':
            *output = '\t';
            return 0;

        default:
            return -1;
    }
}

/**
 * @brief Parses a JSON string into bounded caller storage.
 *
 * UTF-8 bytes occurring directly within the JSON string are preserved.
 *
 * The initial parser supports the common JSON escapes required by the
 * current generated policy index. Unicode \uXXXX escape decoding will be
 * added when the general JSON Core capability is expanded.
 *
 * @param cursor Parser cursor.
 * @param destination Destination string buffer.
 * @param capacity Destination capacity.
 *
 * @return 0 on success, non-zero on malformed or unsupported input.
 */
static int digit_json_parse_string(
    digit_json_cursor_t *cursor,
    char *destination,
    size_t capacity)
{
    size_t length;

    if (cursor == NULL ||
        destination == NULL ||
        capacity == 0U)
    {
        return -1;
    }

    if (*cursor->current != '"')
    {
        return -1;
    }

    cursor->current++;
    length = 0U;

    while (*cursor->current != '\0')
    {
        char value;

        if (*cursor->current == '"')
        {
            destination[length] = '\0';
            cursor->current++;

            return 0;
        }

        /*
         * Unescaped JSON control characters are invalid.
         */
        if ((unsigned char)*cursor->current < 0x20U)
        {
            return -1;
        }

        if (*cursor->current == '\\')
        {
            cursor->current++;

            if (*cursor->current == '\0')
            {
                return -1;
            }

            /*
             * Unicode escapes are not silently misinterpreted.
             *
             * They remain unsupported during this initial index parser
             * stage and therefore produce deterministic rejection.
             */
            if (*cursor->current == 'u')
            {
                return -1;
            }

            if (digit_json_decode_escape(
                    *cursor->current,
                    &value) != 0)
            {
                return -1;
            }

            cursor->current++;
        }
        else
        {
            value = *cursor->current;
            cursor->current++;
        }

        if ((length + 1U) >= capacity)
        {
            return -1;
        }

        destination[length] = value;
        length++;
    }

    return -1;
}

/**
 * @brief Validates SHA-256 hexadecimal syntax.
 *
 * @param value SHA-256 string.
 *
 * @return 1 when exactly 64 hexadecimal characters are present,
 *         otherwise 0.
 */
static int digit_policy_index_sha256_valid(
    const char *value)
{
    size_t index;

    if (value == NULL)
    {
        return 0;
    }

    if (strlen(value) !=
        DIGIT_POLICY_INDEX_SHA256_LENGTH)
    {
        return 0;
    }

    for (index = 0U;
         index < DIGIT_POLICY_INDEX_SHA256_LENGTH;
         index++)
    {
        if (isxdigit(
                (unsigned char)value[index]) == 0)
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Determines whether all required fields were encountered.
 *
 * @param fields Field-presence record.
 *
 * @return 1 when all fields exist exactly once, otherwise 0.
 */
static int digit_policy_index_fields_complete(
    const digit_policy_index_fields_t *fields)
{
    if (fields == NULL)
    {
        return 0;
    }

    return
        fields->root_document_id != 0 &&
        fields->revision_id != 0 &&
        fields->title != 0 &&
        fields->status != 0 &&
        fields->previous_revision != 0 &&
        fields->sha256 != 0;
}

/**
 * @brief Parses one policy index object.
 *
 * Unknown keys and duplicate required keys are rejected.
 *
 * @param cursor Parser cursor.
 * @param record Destination record.
 *
 * @return 0 on success, non-zero on failure.
 */
static int digit_policy_index_parse_record(
    digit_json_cursor_t *cursor,
    digit_policy_index_record_t *record)
{
    digit_policy_index_fields_t fields;

    if (cursor == NULL ||
        record == NULL)
    {
        return -1;
    }

    memset(
        record,
        0,
        sizeof(*record)
    );

    memset(
        &fields,
        0,
        sizeof(fields)
    );

    digit_json_skip_whitespace(
        cursor
    );

    if (*cursor->current != '{')
    {
        return -1;
    }

    cursor->current++;

    for (;;)
    {
        char key[64U];

        digit_json_skip_whitespace(
            cursor
        );

        /*
         * Empty objects are invalid because all contract fields
         * are required.
         */
        if (*cursor->current == '}')
        {
            cursor->current++;

            return -1;
        }

        if (digit_json_parse_string(
                cursor,
                key,
                sizeof(key)) != 0)
        {
            return -1;
        }

        digit_json_skip_whitespace(
            cursor
        );

        if (*cursor->current != ':')
        {
            return -1;
        }

        cursor->current++;

        digit_json_skip_whitespace(
            cursor
        );

        if (strcmp(
                key,
                "root_document_id") == 0)
        {
            if (fields.root_document_id != 0)
            {
                return -1;
            }

            if (digit_json_parse_string(
                    cursor,
                    record->root_document_id,
                    sizeof(record->root_document_id)) != 0)
            {
                return -1;
            }

            fields.root_document_id = 1;
        }
        else if (strcmp(
                     key,
                     "revision_id") == 0)
        {
            if (fields.revision_id != 0)
            {
                return -1;
            }

            if (digit_json_parse_string(
                    cursor,
                    record->revision_id,
                    sizeof(record->revision_id)) != 0)
            {
                return -1;
            }

            fields.revision_id = 1;
        }
        else if (strcmp(
                     key,
                     "title") == 0)
        {
            if (fields.title != 0)
            {
                return -1;
            }

            if (digit_json_parse_string(
                    cursor,
                    record->title,
                    sizeof(record->title)) != 0)
            {
                return -1;
            }

            fields.title = 1;
        }
        else if (strcmp(
                     key,
                     "status") == 0)
        {
            if (fields.status != 0)
            {
                return -1;
            }

            if (digit_json_parse_string(
                    cursor,
                    record->status,
                    sizeof(record->status)) != 0)
            {
                return -1;
            }

            fields.status = 1;
        }
        else if (strcmp(
                     key,
                     "previous_revision") == 0)
        {
            if (fields.previous_revision != 0)
            {
                return -1;
            }

            if (digit_json_parse_string(
                    cursor,
                    record->previous_revision,
                    sizeof(record->previous_revision)) != 0)
            {
                return -1;
            }

            fields.previous_revision = 1;
        }
        else if (strcmp(
                     key,
                     "sha256") == 0)
        {
            if (fields.sha256 != 0)
            {
                return -1;
            }

            if (digit_json_parse_string(
                    cursor,
                    record->sha256,
                    sizeof(record->sha256)) != 0)
            {
                return -1;
            }

            fields.sha256 = 1;
        }
        else
        {
            /*
             * The current machine index contract is strict.
             *
             * Unknown data is not silently accepted because Digit has
             * no established semantics for it at this stage.
             */
            return -1;
        }

        digit_json_skip_whitespace(
            cursor
        );

        if (*cursor->current == ',')
        {
            cursor->current++;
            continue;
        }

        if (*cursor->current == '}')
        {
            cursor->current++;
            break;
        }

        return -1;
    }

    if (digit_policy_index_fields_complete(
            &fields) == 0)
    {
        return -1;
    }

    if (record->root_document_id[0] == '\0' ||
        record->revision_id[0] == '\0' ||
        record->title[0] == '\0' ||
        record->status[0] == '\0' ||
        record->previous_revision[0] == '\0')
    {
        return -1;
    }

    if (digit_policy_index_sha256_valid(
            record->sha256) == 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Parses the complete top-level policy index JSON array.
 *
 * @param json Null-terminated JSON input.
 * @param index Destination index.
 *
 * @return 0 on success, non-zero on malformed input.
 */
static int digit_policy_index_parse(
    const char *json,
    digit_policy_index_t *index)
{
    digit_json_cursor_t cursor;

    if (json == NULL ||
        index == NULL)
    {
        return -1;
    }

    cursor.current = json;

    digit_json_skip_whitespace(
        &cursor
    );

    if (*cursor.current != '[')
    {
        return -1;
    }

    cursor.current++;

    digit_json_skip_whitespace(
        &cursor
    );

    /*
     * An empty authoritative policy index is rejected during this
     * development stage because it establishes no controlled records.
     */
    if (*cursor.current == ']')
    {
        return -1;
    }

    for (;;)
    {
        digit_policy_index_record_t *record;

        if (index->record_count >=
            DIGIT_POLICY_INDEX_MAX_RECORDS)
        {
            return -1;
        }

        record =
            &index->records[
                index->record_count
            ];

        if (digit_policy_index_parse_record(
                &cursor,
                record) != 0)
        {
            return -1;
        }

        index->record_count++;

        digit_json_skip_whitespace(
            &cursor
        );

        if (*cursor.current == ',')
        {
            cursor.current++;

            digit_json_skip_whitespace(
                &cursor
            );

            continue;
        }

        if (*cursor.current == ']')
        {
            cursor.current++;
            break;
        }

        return -1;
    }

    /*
     * Nothing except JSON whitespace may follow the top-level array.
     */
    digit_json_skip_whitespace(
        &cursor
    );

    if (*cursor.current != '\0')
    {
        return -1;
    }

    return 0;
}

int digit_policy_index_init(void)
{
    memset(
        &g_policy_index,
        0,
        sizeof(g_policy_index)
    );

    memset(
        g_policy_index_buffer,
        0,
        sizeof(g_policy_index_buffer)
    );

    g_policy_index_initialized = 1;

    return 0;
}

int digit_policy_index_load(
    const char *directory,
    const char *filename)
{
    size_t bytes_read;

    if (directory == NULL ||
        filename == NULL)
    {
        return -1;
    }

    if (g_policy_index_initialized == 0)
    {
        return -1;
    }

    /*
     * Every load begins from deterministic empty state.
     */
    memset(
        &g_policy_index,
        0,
        sizeof(g_policy_index)
    );

    memset(
        g_policy_index_buffer,
        0,
        sizeof(g_policy_index_buffer)
    );

    bytes_read = 0U;

    if (digit_platform_read_file(
            directory,
            filename,
            g_policy_index_buffer,
            sizeof(g_policy_index_buffer),
            &bytes_read) != 0)
    {
        return -1;
    }

    /*
     * Exact organizational boundary:
     *
     * 8,388,608 bytes     accepted
     * 8,388,609 bytes     rejected
     *
     * The platform reader reserves one additional byte for null
     * termination.
     */
    if (bytes_read >
        DIGIT_STRUCTURED_DOCUMENT_MAX)
    {
        return -1;
    }

    g_policy_index.bytes_read =
        bytes_read;

    if (digit_policy_index_parse(
            g_policy_index_buffer,
            &g_policy_index) != 0)
    {
        memset(
            &g_policy_index,
            0,
            sizeof(g_policy_index)
        );

        return -1;
    }

    return 0;
}

const digit_policy_index_t *digit_policy_index_get(void)
{
    if (g_policy_index_initialized == 0)
    {
        return NULL;
    }

    return &g_policy_index;
}

void digit_policy_index_shutdown(void)
{
    memset(
        &g_policy_index,
        0,
        sizeof(g_policy_index)
    );

    memset(
        g_policy_index_buffer,
        0,
        sizeof(g_policy_index_buffer)
    );

    g_policy_index_initialized = 0;
}