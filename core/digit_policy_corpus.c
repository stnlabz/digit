/**
 * @file digit_policy_corpus.c
 * @brief Policy corpus loading implementation for STN-LABZ Digit Core.
 *
 * This component performs bounded, per-document processing of the local
 * Markdown policy corpus.
 *
 * Current processing path:
 *
 * discovery -> bounded read -> Markdown structural recognition
 *
 * Each document retains its own result state.
 *
 * Corpus storage is statically allocated and owned by Digit Core.
 * Large corpus state is intentionally not placed on the process stack.
 *
 * Corpus loading does not establish policy validity, approval,
 * authority, trust, or operational acceptance.
 */

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "digit_policy.h"
#include "digit_policy_corpus.h"
#include "platform_fs.h"

/**
 * @brief Core-owned policy corpus storage.
 *
 * Static storage prevents the bounded corpus representation from
 * consuming the thread stack.
 */
static digit_policy_corpus_t g_policy_corpus;

/**
 * @brief Indicates whether corpus storage has been initialized.
 */
static int g_policy_corpus_initialized = 0;

/**
 * @brief Context supplied to the platform enumeration callback.
 */
typedef struct digit_policy_corpus_context
{
    const char *directory;
    digit_policy_corpus_t *corpus;

} digit_policy_corpus_context_t;

/**
 * @brief Determines whether a filename has a Markdown .md extension.
 *
 * Extension matching is case-insensitive.
 *
 * @param filename File name to inspect.
 *
 * @return 1 for Markdown filenames, otherwise 0.
 */
static int digit_policy_corpus_is_markdown(
    const char *filename)
{
    size_t length;
    const char *extension;

    if (filename == NULL)
    {
        return 0;
    }

    length = strlen(filename);

    if (length < 3U)
    {
        return 0;
    }

    extension = filename + (length - 3U);

    if (extension[0] != '.')
    {
        return 0;
    }

    if (tolower(
            (unsigned char)extension[1]) != 'm')
    {
        return 0;
    }

    if (tolower(
            (unsigned char)extension[2]) != 'd')
    {
        return 0;
    }

    return 1;
}

/**
 * @brief Copies a filename into bounded document storage.
 *
 * @param destination Destination filename buffer.
 * @param capacity Destination capacity.
 * @param filename Source filename.
 *
 * @return 0 on success, non-zero when the filename does not fit.
 */
static int digit_policy_corpus_copy_filename(
    char *destination,
    size_t capacity,
    const char *filename)
{
    size_t length;

    if (destination == NULL ||
        filename == NULL ||
        capacity == 0U)
    {
        return -1;
    }

    length = strlen(filename);

    if (length >= capacity)
    {
        return -1;
    }

    memcpy(
        destination,
        filename,
        length + 1U
    );

    return 0;
}

/**
 * @brief Processes one filesystem entry.
 *
 * Markdown documents are read and recognized independently.
 *
 * @param filename Discovered filename.
 * @param context Corpus loading context.
 *
 * @return 0 to continue enumeration.
 */
static int digit_policy_corpus_callback(
    const char *filename,
    void *context)
{
    digit_policy_corpus_context_t *loader;
    digit_policy_document_t *document;

    /*
     * This temporary buffer is bounded to one policy document.
     *
     * Unlike the complete corpus representation, this size is suitable
     * for temporary per-document processing during the current stage.
     */
    char policy_buffer[
        DIGIT_POLICY_DOCUMENT_MAX + 1U
    ];

    size_t bytes_read;

    if (filename == NULL ||
        context == NULL)
    {
        return 0;
    }

    loader =
        (digit_policy_corpus_context_t *)context;

    if (digit_policy_corpus_is_markdown(
            filename) == 0)
    {
        loader->corpus->ignored_count++;
        return 0;
    }

    /*
     * Corpus representation has a deterministic upper bound.
     */
    if (loader->corpus->document_count >=
        DIGIT_POLICY_CORPUS_MAX)
    {
        loader->corpus->failed_count++;
        return 0;
    }

    document =
        &loader->corpus->documents[
            loader->corpus->document_count
        ];

    memset(
        document,
        0,
        sizeof(*document)
    );

    document->state =
        DIGIT_POLICY_DOCUMENT_DISCOVERED;

    loader->corpus->document_count++;

    if (digit_policy_corpus_copy_filename(
            document->filename,
            sizeof(document->filename),
            filename) != 0)
    {
        document->state =
            DIGIT_POLICY_DOCUMENT_FAILED;

        loader->corpus->failed_count++;

        return 0;
    }

    if (digit_policy_read(
            loader->directory,
            filename,
            policy_buffer,
            sizeof(policy_buffer),
            &bytes_read) != 0)
    {
        document->state =
            DIGIT_POLICY_DOCUMENT_FAILED;

        loader->corpus->failed_count++;

        return 0;
    }

    document->bytes_read = bytes_read;

    document->state =
        DIGIT_POLICY_DOCUMENT_READ;

    if (digit_markdown_recognize(
            policy_buffer,
            &document->structure) != 0)
    {
        document->state =
            DIGIT_POLICY_DOCUMENT_FAILED;

        loader->corpus->failed_count++;

        return 0;
    }

    document->state =
        DIGIT_POLICY_DOCUMENT_RECOGNIZED;

    loader->corpus->recognized_count++;

    return 0;
}

int digit_policy_corpus_init(void)
{
    memset(
        &g_policy_corpus,
        0,
        sizeof(g_policy_corpus)
    );

    g_policy_corpus_initialized = 1;

    return 0;
}

int digit_policy_corpus_load(
    const char *directory)
{
    digit_policy_corpus_context_t context;

    if (directory == NULL)
    {
        return -1;
    }

    if (g_policy_corpus_initialized == 0)
    {
        return -1;
    }

    /*
     * Every load begins from deterministic empty state.
     */
    memset(
        &g_policy_corpus,
        0,
        sizeof(g_policy_corpus)
    );

    context.directory = directory;
    context.corpus = &g_policy_corpus;

    if (digit_platform_enumerate_files(
            directory,
            digit_policy_corpus_callback,
            &context) != 0)
    {
        return -1;
    }

    return 0;
}

const digit_policy_corpus_t *digit_policy_corpus_get(void)
{
    if (g_policy_corpus_initialized == 0)
    {
        return NULL;
    }

    return &g_policy_corpus;
}

void digit_policy_corpus_shutdown(void)
{
    memset(
        &g_policy_corpus,
        0,
        sizeof(g_policy_corpus)
    );

    g_policy_corpus_initialized = 0;
}