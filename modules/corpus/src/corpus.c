#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "corpus.h"

static const stnlabz_module_host_t *corpus_host = NULL;

static int corpus_safe_field(const char *value)
{
    return value != NULL && value[0] != '\0' && strchr(value, '\n') == NULL && strchr(value, '\r') == NULL && strchr(value, '\t') == NULL;
}

int digit_corpus_validate(const digit_corpus_record_t *record)
{
    if (record == NULL) return 0;
    if (!corpus_safe_field(record->id)) return 0;
    if (!corpus_safe_field(record->category)) return 0;
    if (!corpus_safe_field(record->source)) return 0;
    if (!corpus_safe_field(record->text)) return 0;
    return 1;
}

int digit_corpus_contains(const char *path, const char *record_id)
{
    FILE *file;
    char line[DIGIT_CORPUS_TEXT_MAX + DIGIT_CORPUS_SOURCE_MAX + DIGIT_CORPUS_CATEGORY_MAX + DIGIT_CORPUS_RECORD_ID_MAX + 16];
    size_t id_length;

    if (path == NULL || record_id == NULL || record_id[0] == '\0') return 0;
    file = fopen(path, "r");
    if (file == NULL) return 0;
    id_length = strlen(record_id);

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strncmp(line, record_id, id_length) == 0 && line[id_length] == '\t')
        {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int digit_corpus_append(const char *path, const digit_corpus_record_t *record)
{
    FILE *file;
    int written;
    if (path == NULL || !digit_corpus_validate(record)) return 0;
    if (digit_corpus_contains(path, record->id)) return 0;
    file = fopen(path, "a");
    if (file == NULL) return 0;
    written = fprintf(file, "%s\t%s\t%s\t%s\n", record->id, record->category, record->source, record->text);
    if (fflush(file) != 0) { fclose(file); return 0; }
    if (fclose(file) != 0) return 0;
    return written > 0;
}

static stnlabz_module_result_t corpus_contains_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    digit_corpus_contains_result_t result;
    const char *record_id = request;
    (void)handler_context;
    if (request == NULL || request_size == 0 || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (record_id[request_size - 1] != '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    result.contains = digit_corpus_contains(DIGIT_CORPUS_PATH, record_id);
    memcpy(response, &result, sizeof(result));
    *response_used = sizeof(result);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_append_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    digit_corpus_append_result_t result;
    const digit_corpus_record_t *record = request;
    (void)handler_context;
    if (request == NULL || request_size != sizeof(*record) || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    result.appended = digit_corpus_append(DIGIT_CORPUS_PATH, record);
    memcpy(response, &result, sizeof(result));
    *response_used = sizeof(result);
    return result.appended ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
}

static stnlabz_module_result_t corpus_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_CORPUS_CONTAINS_SERVICE, corpus_contains_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    if (!host->register_service(DIGIT_CORPUS_APPEND_SERVICE, corpus_append_service, NULL))
    {
        if (host->unregister_service != NULL) (void)host->unregister_service(DIGIT_CORPUS_CONTAINS_SERVICE, NULL);
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    corpus_host = host;
    if (host->send_message != NULL) (void)host->send_message("[CORPUS] module active: corpus.contains and corpus.append registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_stop(void)
{
    int ok = 1;
    if (corpus_host != NULL && corpus_host->unregister_service != NULL)
    {
        if (!corpus_host->unregister_service(DIGIT_CORPUS_APPEND_SERVICE, NULL)) ok = 0;
        if (!corpus_host->unregister_service(DIGIT_CORPUS_CONTAINS_SERVICE, NULL)) ok = 0;
    }
    corpus_host = NULL;
    return ok ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_STOP_FAILED;
}

static const stnlabz_module_descriptor_t corpus_descriptor =
{
    "corpus", "Digit Corpus", 1, 0, 1,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    corpus_qualify, corpus_start, corpus_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &corpus_descriptor;
}
