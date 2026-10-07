#ifndef DIGIT_REASONING_H
#define DIGIT_REASONING_H

#include <stddef.h>
#include "module.h"

#define DIGIT_REASONING_SERVICE "reasoning.evaluate"
#define DIGIT_REASONING_EXPLAIN_SERVICE "reasoning.explain"
#define DIGIT_REASONING_COMPARE_SERVICE "reasoning.compare"

#define DIGIT_REASONING_SUBJECT_MAX 256
#define DIGIT_REASONING_EVIDENCE_MAX 6
#define DIGIT_REASONING_EVIDENCE_TEXT_MAX 4096
#define DIGIT_REASONING_EXPLANATION_MAX 4096

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

/*
 * [AI:GPT-5.6 Sol | 2026-10-07T00:18:00Z]
 * Public deterministic EXPLAIN contract. Reasoning receives only the
 * established subject and already-selected authorized evidence. It may
 * organize that evidence, but it may not introduce unsupported knowledge.
 */
typedef struct
{
    char subject[DIGIT_REASONING_SUBJECT_MAX];
    size_t evidence_count;
    char evidence[DIGIT_REASONING_EVIDENCE_MAX][DIGIT_REASONING_EVIDENCE_TEXT_MAX];
} digit_reasoning_explain_request_t;

typedef struct
{
    int explained;
    size_t evidence_used;
    char explanation[DIGIT_REASONING_EXPLANATION_MAX];
} digit_reasoning_explain_result_t;

typedef struct
{
    char left[DIGIT_REASONING_SUBJECT_MAX];
    char right[DIGIT_REASONING_SUBJECT_MAX];
    size_t evidence_count;
    char evidence[DIGIT_REASONING_EVIDENCE_MAX][DIGIT_REASONING_EVIDENCE_TEXT_MAX];
} digit_reasoning_compare_request_t;

typedef struct
{
    int compared;
    size_t left_evidence_used;
    size_t right_evidence_used;
    char comparison[DIGIT_REASONING_EXPLANATION_MAX];
} digit_reasoning_compare_result_t;

int digit_reasoning_evaluate(
    const char *context,
    digit_reasoning_result_t *result
);

const char *digit_relevance_string(digit_relevance_t relevance);
const char *digit_context_category_string(digit_context_category_t category);
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
