#ifndef DIGIT_RESPONSE_H
#define DIGIT_RESPONSE_H

#include "module.h"

#define DIGIT_RESPONSE_SERVICE "response.answer"
#define DIGIT_RESPONSE_OBSERVED_SERVICE "response.answer_observed"
#define DIGIT_RESPONSE_QUESTION_MAX 4096
#define DIGIT_RESPONSE_ANSWER_MAX 4096
/* Diagnostic reason codes describe processing outcomes, not verified root causes. */
typedef enum {
    DIGIT_RESPONSE_PHASE_UNOBSERVED=0,
    DIGIT_RESPONSE_PHASE_INPUT_REJECTED,
    DIGIT_RESPONSE_PHASE_EVIDENCE_UNAVAILABLE,
    DIGIT_RESPONSE_PHASE_NO_RECORDS,
    DIGIT_RESPONSE_PHASE_NO_SELECTION,
    DIGIT_RESPONSE_PHASE_NO_COMPOSITION,
    DIGIT_RESPONSE_PHASE_VALIDATOR_UNAVAILABLE,
    DIGIT_RESPONSE_PHASE_VALIDATOR_REJECTED,
    DIGIT_RESPONSE_PHASE_APPROVED,
    DIGIT_RESPONSE_PHASE_DIRECT_RESPONSE
} digit_response_phase_t;

typedef struct
{
    char question[DIGIT_RESPONSE_QUESTION_MAX];
} digit_response_request_t;

typedef struct
{
    int answered;
    unsigned int evidence_count;
    char answer[DIGIT_RESPONSE_ANSWER_MAX];
} digit_response_result_t;

/* Additive service contract. Prefix must match the original result. */
typedef struct
{
    int answered;
    unsigned int evidence_count;
    char answer[DIGIT_RESPONSE_ANSWER_MAX];
    unsigned int retrieved_count;
    unsigned int selected_count;
    digit_response_phase_t diagnostic_phase;
} digit_response_observed_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
