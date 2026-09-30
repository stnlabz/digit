#ifndef DIGIT_RESPONSE_H
#define DIGIT_RESPONSE_H

#include "module.h"

#define DIGIT_RESPONSE_SERVICE "response.answer"
#define DIGIT_RESPONSE_QUESTION_MAX 4096
#define DIGIT_RESPONSE_ANSWER_MAX 4096

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

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
