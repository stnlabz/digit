#ifndef DIGIT_INTENT_H
#define DIGIT_INTENT_H

#include <stddef.h>
#include "module.h"

#define DIGIT_INTENT_SERVICE "intent.interpret"
#define DIGIT_INTENT_TEXT_MAX 4096
#define DIGIT_INTENT_SUBJECT_MAX 256
#define DIGIT_INTENT_REASON_MAX 256

typedef enum
{
    DIGIT_INTENT_UNKNOWN = 0,
    DIGIT_INTENT_CONVERSATION,
    DIGIT_INTENT_FACT,
    DIGIT_INTENT_DEFINE,
    DIGIT_INTENT_EXPLAIN,
    DIGIT_INTENT_COMPARE,
    DIGIT_INTENT_WHY,
    DIGIT_INTENT_HOW,
    DIGIT_INTENT_STATUS,
    DIGIT_INTENT_ACTION,
    DIGIT_INTENT_AMBIGUOUS
} digit_intent_class_t;

typedef enum
{
    DIGIT_INTENT_TARGET_UNKNOWN = 0,
    DIGIT_INTENT_TARGET_SOCIAL,
    DIGIT_INTENT_TARGET_KNOWLEDGE,
    DIGIT_INTENT_TARGET_RUNTIME,
    DIGIT_INTENT_TARGET_CAPABILITY
} digit_intent_target_t;

typedef struct
{
    char text[DIGIT_INTENT_TEXT_MAX];
} digit_intent_request_t;

typedef struct
{
    digit_intent_class_t intent;
    digit_intent_target_t target;
    unsigned int established;
    char subject[DIGIT_INTENT_SUBJECT_MAX];
    char reason[DIGIT_INTENT_REASON_MAX];
} digit_intent_result_t;

void digit_intent_interpret(const char *text, digit_intent_result_t *result);
const char *digit_intent_class_string(digit_intent_class_t intent);
const char *digit_intent_target_string(digit_intent_target_t target);
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
