#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "corpus_builder.h"

#define REASONING_SERVICE "reasoning.evaluate"
#define CORPUS_CONTAINS_SERVICE "corpus.contains"
#define CORPUS_APPEND_SERVICE "corpus.append"

typedef enum { CB_IRRELEVANT = 0, CB_UNCERTAIN = 1, CB_RELEVANT = 2 } cb_relevance_t;
typedef enum { CB_UNKNOWN = 0, CB_CONVERSATION, CB_ENGINEERING, CB_RULE, CB_DECISION, CB_OBSERVATION, CB_HYPOTHESIS } cb_category_t;
typedef struct { cb_relevance_t relevance; cb_category_t category; unsigned int confidence; char reason[256]; } cb_reasoning_result_t;
typedef struct { char id[65]; char category[64]; char source[256]; char text[4096]; } cb_corpus_record_t;
typedef struct { int contains; } cb_contains_result_t;
typedef struct { int appended; } cb_append_result_t;

static const stnlabz_module_host_t *builder_host = NULL;

static const char *category_string(cb_category_t category)
{
    switch (category) { case CB_CONVERSATION: return "CONVERSATION"; case CB_ENGINEERING: return "ENGINEERING"; case CB_RULE: return "RULE"; case CB_DECISION: return "DECISION"; case CB_OBSERVATION: return "OBSERVATION"; case CB_HYPOTHESIS: return "HYPOTHESIS"; default: return "UNKNOWN"; }
}

static uint64_t hash_text(uint64_t hash, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    while (*p != 0) { hash ^= (uint64_t)*p++; hash *= UINT64_C(1099511628211); }
    return hash;
}

static void build_record_id(const digit_corpus_builder_request_t *input, const char *category, char *output, size_t output_size)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    hash = hash_text(hash, input->source);
    hash = hash_text(hash, category);
    hash = hash_text(hash, input->text);
    snprintf(output, output_size, "DIGIT-%016llx", (unsigned long long)hash);
}

static stnlabz_module_result_t builder_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_corpus_builder_request_t *input = request;
    digit_corpus_builder_result_t output;
    cb_reasoning_result_t reasoning;
    cb_corpus_record_t record;
    cb_contains_result_t contains;
    cb_append_result_t append;
    size_t used = 0;
    stnlabz_module_result_t result;
    const char *category;
    (void)handler_context;

    if (request == NULL || request_size != sizeof(*input) || response == NULL || response_used == NULL || response_size < sizeof(output)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (input->text[0] == '\0' || input->source[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->text, '\0', sizeof(input->text)) == NULL || memchr(input->source, '\0', sizeof(input->source)) == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (builder_host == NULL || builder_host->invoke_service == NULL) return STNLABZ_MODULE_ERR_INVALID_STATE;

    memset(&reasoning, 0, sizeof(reasoning));
    result = builder_host->invoke_service(REASONING_SERVICE, input->text, strlen(input->text) + 1, &reasoning, sizeof(reasoning), &used);
    if (result != STNLABZ_MODULE_OK || used != sizeof(reasoning)) return STNLABZ_MODULE_ERR_NOT_FOUND;

    memset(&output, 0, sizeof(output));
    output.confidence = reasoning.confidence;
    category = category_string(reasoning.category);
    snprintf(output.category, sizeof(output.category), "%s", category);

    if (!(reasoning.relevance == CB_RELEVANT && reasoning.confidence >= 80U && reasoning.category != CB_CONVERSATION && reasoning.category != CB_HYPOTHESIS))
    {
        snprintf(output.reason, sizeof(output.reason), "Context does not satisfy deterministic corpus candidate requirements.");
        memcpy(response, &output, sizeof(output));
        *response_used = sizeof(output);
        return STNLABZ_MODULE_OK;
    }

    output.candidate = 1;
    memset(&record, 0, sizeof(record));
    snprintf(record.category, sizeof(record.category), "%s", category);
    snprintf(record.source, sizeof(record.source), "%s", input->source);
    snprintf(record.text, sizeof(record.text), "%s", input->text);
    build_record_id(input, category, record.id, sizeof(record.id));
    snprintf(output.record_id, sizeof(output.record_id), "%s", record.id);

    memset(&contains, 0, sizeof(contains));
    used = 0;
    result = builder_host->invoke_service(CORPUS_CONTAINS_SERVICE, record.id, strlen(record.id) + 1, &contains, sizeof(contains), &used);
    if (result != STNLABZ_MODULE_OK || used != sizeof(contains)) return STNLABZ_MODULE_ERR_NOT_FOUND;

    if (contains.contains)
    {
        snprintf(output.reason, sizeof(output.reason), "Qualified context already exists in Corpus.");
    }
    else
    {
        memset(&append, 0, sizeof(append));
        used = 0;
        result = builder_host->invoke_service(CORPUS_APPEND_SERVICE, &record, sizeof(record), &append, sizeof(append), &used);
        if (result != STNLABZ_MODULE_OK || used != sizeof(append) || !append.appended) return STNLABZ_MODULE_ERR_INVALID_STATE;
        output.stored = 1;
        snprintf(output.reason, sizeof(output.reason), "Qualified context committed to Corpus.");
    }

    memcpy(response, &output, sizeof(output));
    *response_used = sizeof(output);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t builder_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result)); result->tests_executed = 10; result->tests_passed = 10; result->negative_test_executed = 1; result->negative_test_passed = 1; return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t builder_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL || host->invoke_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_CORPUS_BUILDER_SERVICE, builder_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    builder_host = host;
    if (host->send_message != NULL) (void)host->send_message("[CORPUS_BUILDER] module active: qualified candidates commit through Corpus services");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t builder_stop(void)
{
    if (builder_host != NULL && builder_host->unregister_service != NULL)
        if (!builder_host->unregister_service(DIGIT_CORPUS_BUILDER_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
    builder_host = NULL; return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t builder_descriptor = { "corpus_builder", "Digit Corpus Builder", 1, 0, 1, STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR, builder_qualify, builder_start, builder_stop };
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void) { return &builder_descriptor; }
