#ifndef DIGIT_INTERPRETATION_H
#define DIGIT_INTERPRETATION_H

#include <stddef.h>
#include "module.h"

#define DIGIT_INTERPRETATION_SERVICE "interpretation.resolve"
#define DIGIT_INTERPRETATION_TEXT_MAX 4096
#define DIGIT_INTERPRETATION_REASON_MAX 256

typedef struct
{
    char text[DIGIT_INTERPRETATION_TEXT_MAX];
} digit_interpretation_request_t;

typedef struct
{
    unsigned int resolved;
    unsigned int substitutions;
    char normalized[DIGIT_INTERPRETATION_TEXT_MAX];
    char reason[DIGIT_INTERPRETATION_REASON_MAX];
} digit_interpretation_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
