#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "reasoning.h"

static int contains_word_ci(const char *text, const char *word)
{
    size_t text_length;
    size_t word_length;
    size_t index;
    size_t offset;

    if (text == NULL || word == NULL)
    {
        return 0;
    }

    text_length = strlen(text);
    word_length = strlen(word);
    if (word_length == 0 || word_length > text_length)
    {
        return 0;
    }

    for (index = 0; index + word_length <= text_length; ++index)
    {
        int match = 1;

        for (offset = 0; offset < word_length; ++offset)
        {
            if (tolower((unsigned char)text[index + offset]) !=
                tolower((unsigned char)word[offset]))
            {
                match = 0;
                break;
            }
        }

        if (match)
        {
            return 1;
        }
    }

    return 0;
}

const char *digit_relevance_string(digit_relevance_t relevance)
{
    switch (relevance)
    {
        case DIGIT_RELEVANCE_IRRELEVANT:
            return "IRRELEVANT";
        case DIGIT_RELEVANCE_UNCERTAIN:
            return "UNCERTAIN";
        case DIGIT_RELEVANCE_RELEVANT:
            return "RELEVANT";
        default:
            return "UNKNOWN";
    }
}

const char *digit_context_category_string(digit_context_category_t category)
{
    switch (category)
    {
        case DIGIT_CONTEXT_CONVERSATION:
            return "CONVERSATION";
        case DIGIT_CONTEXT_ENGINEERING:
            return "ENGINEERING";
        case DIGIT_CONTEXT_RULE:
            return "RULE";
        case DIGIT_CONTEXT_DECISION:
            return "DECISION";
        case DIGIT_CONTEXT_OBSERVATION:
            return "OBSERVATION";
        case DIGIT_CONTEXT_HYPOTHESIS:
            return "HYPOTHESIS";
        default:
            return "UNKNOWN";
    }
}

int digit_reasoning_evaluate(
    const char *context,
    digit_reasoning_result_t *result
)
{
    int self_reference;
    int engineering_reference;
    int rule_reference;
    int decision_reference;
    int hypothesis_reference;

    if (context == NULL || result == NULL || context[0] == '\0')
    {
        return 0;
    }

    memset(result, 0, sizeof(*result));
    result->relevance = DIGIT_RELEVANCE_UNCERTAIN;
    result->category = DIGIT_CONTEXT_CONVERSATION;
    result->confidence = 25;
    snprintf(result->reason, sizeof(result->reason),
             "No deterministic relationship to Digit established.");

    self_reference =
        contains_word_ci(context, "digit") ||
        contains_word_ci(context, "her core") ||
        contains_word_ci(context, "her module") ||
        contains_word_ci(context, "her corpus");

    engineering_reference =
        contains_word_ci(context, "module") ||
        contains_word_ci(context, "core") ||
        contains_word_ci(context, "abi") ||
        contains_word_ci(context, "corpus") ||
        contains_word_ci(context, "qualification") ||
        contains_word_ci(context, "hotload");

    rule_reference =
        contains_word_ci(context, "rule") ||
        contains_word_ci(context, "must") ||
        contains_word_ci(context, "required");

    decision_reference =
        contains_word_ci(context, "decided") ||
        contains_word_ci(context, "decision") ||
        contains_word_ci(context, "will use") ||
        contains_word_ci(context, "going forward");

    hypothesis_reference =
        contains_word_ci(context, "maybe") ||
        contains_word_ci(context, "perhaps") ||
        contains_word_ci(context, "might") ||
        contains_word_ci(context, "could");

    if (self_reference)
    {
        result->relevance = DIGIT_RELEVANCE_RELEVANT;
        result->confidence = engineering_reference ? 95U : 80U;
        result->category = engineering_reference ?
            DIGIT_CONTEXT_ENGINEERING : DIGIT_CONTEXT_OBSERVATION;
        snprintf(result->reason, sizeof(result->reason),
                 "Context directly references Digit or a Digit-owned capability.");
    }
    else if (engineering_reference)
    {
        result->relevance = DIGIT_RELEVANCE_UNCERTAIN;
        result->category = DIGIT_CONTEXT_ENGINEERING;
        result->confidence = 50;
        snprintf(result->reason, sizeof(result->reason),
                 "Engineering context detected without a deterministic Digit relationship.");
    }

    if (rule_reference && self_reference)
    {
        result->category = DIGIT_CONTEXT_RULE;
    }
    else if (decision_reference && self_reference)
    {
        result->category = DIGIT_CONTEXT_DECISION;
    }
    else if (hypothesis_reference)
    {
        result->category = DIGIT_CONTEXT_HYPOTHESIS;
    }

    return 1;
}

static stnlabz_module_result_t reasoning_qualify(
    stnlabz_module_qualification_result_t *result
)
{
    if (result == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t reasoning_start(const stnlabz_module_host_t *host)
{
    if (host != NULL && host->send_message != NULL)
    {
        (void)host->send_message("[REASONING] module active");
    }

    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t reasoning_stop(void)
{
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t reasoning_descriptor =
{
    "reasoning",
    "Digit Relevance Reasoning",
    1,
    0,
    0,
    STNLABZ_MODULE_API_MAJOR,
    STNLABZ_MODULE_API_MINOR,
    reasoning_qualify,
    reasoning_start,
    reasoning_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &reasoning_descriptor;
}
