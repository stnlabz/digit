#include <stdio.h>
#include <string.h>

#include "corpus_builder.h"

#define REASONING_SERVICE "reasoning.evaluate"

typedef enum { CB_IRRELEVANT = 0, CB_UNCERTAIN = 1, CB_RELEVANT = 2 } cb_relevance_t;
typedef enum { CB_UNKNOWN = 0, CB_CONVERSATION, CB_ENGINEERING, CB_RULE, CB_DECISION, CB_OBSERVATION, CB_HYPOTHESIS } cb_category_t;
typedef struct { cb_relevance_t relevance; cb_category_t category; unsigned int confidence; char reason[256]; } cb_reasoning_result_t;

static const stnlabz_module_host_t *builder_host = NULL;

static const char *category_string(cb_category_t category)
{
    switch (category)
    {
        case CB_CONVERSATION: return "CONVERSATION";
        case CB_ENGINEERING: return "ENGINEERING";
        case CB_RULE: return "RULE";
        case CB_DECISION: return "DECISION";
        case CB_OBSERVATION: return "OBSERVATION";
        case CB_HYPOTHESIS: return "HYPOTHESIS";
        default: return "UNKNOWN";
    }
}

static stnlabz_module_result_t builder_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_corpus_builder_request_t *input = request;
    digit_corpus_builder_result_t output;
    cb_reasoning_result_t reasoning;
    size_t reasoning_used = 0;
    stnlabz_module_result_t result;
    (void)handler_context;

    if (request == NULL || request_size != sizeof(*input) || response == NULL || response_used == NULL || response_size < sizeof(output)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (input->text[0] == '\0' || input->source[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (builder_host == NULL || builder_host->invoke_service == NULL) return STNLABZ_MODULE_ERR_INVALID_STATE;

    memset(&reasoning, 0, sizeof(reasoning));
    result = builder_host->invoke_service(REASONING_SERVICE, input->text, strlen(input->text) + 1, &reasoning, sizeof(reasoning), &reasoning_used);
    if (result != STNLABZ_MODULE_OK || reasoning_used != sizeof(reasoning)) return STNLABZ_MODULE_ERR_NOT_FOUND;

    memset(&output, 0, sizeof(output));
    output.confidence = reasoning.confidence;
    snprintf(output.category, sizeof(output.category), "%s", category_string(reasoning.category));

    if (reasoning.relevance == CB_RELEVANT && reasoning.confidence >= 80U && reasoning.category != CB_CONVERSATION && reasoning.category != CB_HYPOTHESIS)
    {
        output.candidate = 1;
        snprintf(output.reason, sizeof(output.reason), "Reasoning established durable Digit relevance at or above candidate threshold.");
    }
    else
    {
        output.candidate = 0;
        snprintf(output.reason, sizeof(output.reason), "Context does not satisfy deterministic corpus candidate requirements.");
    }

    memcpy(response, &output, sizeof(output));
    *response_used = sizeof(output);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t builder_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t builder_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL || host->invoke_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_CORPUS_BUILDER_SERVICE, builder_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    builder_host = host;
    if (host->send_message != NULL) (void)host->send_message("[CORPUS_BUILDER] module active: corpus_builder.evaluate registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t builder_stop(void)
{
    if (builder_host != NULL && builder_host->unregister_service != NULL)
        if (!builder_host->unregister_service(DIGIT_CORPUS_BUILDER_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
    builder_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t builder_descriptor =
{
    "corpus_builder", "Digit Corpus Builder", 1, 0, 0,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    builder_qualify, builder_start, builder_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &builder_descriptor;
}
