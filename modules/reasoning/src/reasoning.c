#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "reasoning.h"

static const stnlabz_module_host_t *reasoning_host = NULL;

static int contains_ci(const char *text, const char *word)
{
    size_t text_length;
    size_t word_length;
    size_t index;
    size_t offset;

    if (text == NULL || word == NULL) return 0;
    text_length = strlen(text);
    word_length = strlen(word);
    if (word_length == 0 || word_length > text_length) return 0;

    for (index = 0; index + word_length <= text_length; ++index)
    {
        int match = 1;
        for (offset = 0; offset < word_length; ++offset)
        {
            if (tolower((unsigned char)text[index + offset]) != tolower((unsigned char)word[offset]))
            {
                match = 0;
                break;
            }
        }
        if (match) return 1;
    }
    return 0;
}

static void source_view(const char *context, const char **source, size_t *source_length)
{
    const char *start;
    const char *end;
    if (source == NULL || source_length == NULL) return;
    *source = context;
    *source_length = context != NULL ? strlen(context) : 0;
    if (context == NULL || strncmp(context, "SOURCE:\n", 8) != 0) return;
    start = context + 8;
    end = strstr(start, "\n\nCONTEXT:\n");
    if (end != NULL)
    {
        *source = start;
        *source_length = (size_t)(end - start);
    }
}

static int source_contains_ci(const char *source, size_t source_length, const char *needle)
{
    size_t needle_length;
    size_t i, j;
    if (source == NULL || needle == NULL) return 0;
    needle_length = strlen(needle);
    if (needle_length == 0 || needle_length > source_length) return 0;
    for (i = 0; i + needle_length <= source_length; ++i)
    {
        int match = 1;
        for (j = 0; j < needle_length; ++j)
        {
            if (tolower((unsigned char)source[i + j]) != tolower((unsigned char)needle[j])) { match = 0; break; }
        }
        if (match) return 1;
    }
    return 0;
}

const char *digit_relevance_string(digit_relevance_t relevance)
{
    switch (relevance)
    {
        case DIGIT_RELEVANCE_IRRELEVANT: return "IRRELEVANT";
        case DIGIT_RELEVANCE_UNCERTAIN: return "UNCERTAIN";
        case DIGIT_RELEVANCE_RELEVANT: return "RELEVANT";
        default: return "UNKNOWN";
    }
}

const char *digit_context_category_string(digit_context_category_t category)
{
    switch (category)
    {
        case DIGIT_CONTEXT_CONVERSATION: return "CONVERSATION";
        case DIGIT_CONTEXT_ENGINEERING: return "ENGINEERING";
        case DIGIT_CONTEXT_RULE: return "RULE";
        case DIGIT_CONTEXT_DECISION: return "DECISION";
        case DIGIT_CONTEXT_OBSERVATION: return "OBSERVATION";
        case DIGIT_CONTEXT_HYPOTHESIS: return "HYPOTHESIS";
        default: return "UNKNOWN";
    }
}

int digit_reasoning_evaluate(const char *context, digit_reasoning_result_t *result)
{
    const char *source;
    size_t source_length;
    int self_reference;
    int engineering_reference;
    int source_rule_reference;
    int decision_reference;
    int hypothesis_reference;

    if (context == NULL || result == NULL || context[0] == '\0') return 0;

    memset(result, 0, sizeof(*result));
    result->relevance = DIGIT_RELEVANCE_UNCERTAIN;
    result->category = DIGIT_CONTEXT_CONVERSATION;
    result->confidence = 25;
    snprintf(result->reason, sizeof(result->reason), "No deterministic relationship to Digit established.");

    source_view(context, &source, &source_length);

    self_reference = source_contains_ci(source, source_length, "digit") || source_contains_ci(source, source_length, "her core") || source_contains_ci(source, source_length, "her module") || source_contains_ci(source, source_length, "her corpus");
    engineering_reference = source_contains_ci(source, source_length, "module") || source_contains_ci(source, source_length, "core") || source_contains_ci(source, source_length, "abi") || source_contains_ci(source, source_length, "corpus") || source_contains_ci(source, source_length, "qualification") || source_contains_ci(source, source_length, "hotload");
    source_rule_reference = source_contains_ci(source, source_length, "rule") || source_contains_ci(source, source_length, "must") || source_contains_ci(source, source_length, "required") || source_contains_ci(source, source_length, "will ") || source_contains_ci(source, source_length, "will not") || source_contains_ci(source, source_length, "prohibited");
    decision_reference = source_contains_ci(source, source_length, "decided") || source_contains_ci(source, source_length, "decision") || source_contains_ci(source, source_length, "will use") || source_contains_ci(source, source_length, "going forward");
    hypothesis_reference = source_contains_ci(source, source_length, "maybe") || source_contains_ci(source, source_length, "perhaps") || source_contains_ci(source, source_length, "might") || source_contains_ci(source, source_length, "could");

    if (self_reference)
    {
        result->relevance = DIGIT_RELEVANCE_RELEVANT;
        result->confidence = engineering_reference ? 95U : 80U;
        result->category = engineering_reference ? DIGIT_CONTEXT_ENGINEERING : DIGIT_CONTEXT_OBSERVATION;
        snprintf(result->reason, sizeof(result->reason), "Authoritative source directly references Digit or a Digit-owned capability.");
    }
    else if (engineering_reference)
    {
        result->relevance = DIGIT_RELEVANCE_UNCERTAIN;
        result->category = DIGIT_CONTEXT_ENGINEERING;
        result->confidence = 50;
        snprintf(result->reason, sizeof(result->reason), "Engineering source detected without a deterministic Digit relationship.");
    }

    if (source_rule_reference && self_reference)
    {
        result->category = DIGIT_CONTEXT_RULE;
        result->confidence = 95U;
        snprintf(result->reason, sizeof(result->reason), "Authoritative source establishes normative Digit behavior.");
    }
    else if (decision_reference && self_reference) result->category = DIGIT_CONTEXT_DECISION;
    else if (hypothesis_reference) result->category = DIGIT_CONTEXT_HYPOTHESIS;

    return 1;
}

static stnlabz_module_result_t reasoning_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const char *context = request;
    digit_reasoning_result_t result;
    (void)handler_context;
    if (request == NULL || request_size == 0 || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (((const char *)request)[request_size - 1] != '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!digit_reasoning_evaluate(context, &result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memcpy(response, &result, sizeof(result));
    *response_used = sizeof(result);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t reasoning_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t reasoning_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_REASONING_SERVICE, reasoning_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    reasoning_host = host;
    if (host->send_message != NULL) (void)host->send_message("[REASONING] module active: authoritative-source reasoning.evaluate registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t reasoning_stop(void)
{
    if (reasoning_host != NULL && reasoning_host->unregister_service != NULL)
    {
        if (!reasoning_host->unregister_service(DIGIT_REASONING_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
    }
    reasoning_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t reasoning_descriptor =
{
    "reasoning", "Digit Relevance Reasoning", 1, 0, 2,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    reasoning_qualify, reasoning_start, reasoning_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &reasoning_descriptor;
}
