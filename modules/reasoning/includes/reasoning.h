#ifndef DIGIT_REASONING_H
#define DIGIT_REASONING_H

#include <stddef.h>
#include "module.h"

#define DIGIT_REASONING_SERVICE "reasoning.evaluate"

typedef enum
{
    DIGIT_RELEVANCE_IRRELEVANT = 0,
    DIGIT_RELEVANCE_UNCERTAIN = 1,
    DIGIT_RELEVANCE_RELEVANT = 2
} digit_relevance_t;

typedef enum
{
    DIGIT_CONTEXT_UNKNOWN = 0,
    DIGIT_CONTEXT_CONVERSATION,
    DIGIT_CONTEXT_ENGINEERING,
    DIGIT_CONTEXT_RULE,
    DIGIT_CONTEXT_DECISION,
    DIGIT_CONTEXT_OBSERVATION,
    DIGIT_CONTEXT_HYPOTHESIS
} digit_context_category_t;

typedef struct
{
    digit_relevance_t relevance;
    digit_context_category_t category;
    unsigned int confidence;
    char reason[256];
} digit_reasoning_result_t;

int digit_reasoning_evaluate(
    const char *context,
    digit_reasoning_result_t *result
);

const char *digit_relevance_string(digit_relevance_t relevance);
const char *digit_context_category_string(digit_context_category_t category);
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
