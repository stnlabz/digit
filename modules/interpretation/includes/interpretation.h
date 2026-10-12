#ifndef DIGIT_INTERPRETATION_H
#define DIGIT_INTERPRETATION_H

#include <stddef.h>
#include "module.h"

#define DIGIT_INTERPRETATION_SERVICE "interpretation.resolve"
#define DIGIT_INTERPRETATION_TEXT_MAX 4096
#define DIGIT_INTERPRETATION_REASON_MAX 256
#define DIGIT_INTERPRETATION_DEFINITION_SERVICE "interpretation.definition"
#define DIGIT_INTERPRETATION_TERM_MAX 128
#define DIGIT_INTERPRETATION_TRACE_MAX 4
/* Read-only, request-local observations from actual lexical Corpus searches. */
typedef struct {
    char term[64];
    char selected_record_id[65];
    char decision[48];
    unsigned int returned;
    unsigned int accepted;
    unsigned int query_ok;
} digit_interpretation_lookup_t;

typedef struct
{
    char term[DIGIT_INTERPRETATION_TERM_MAX];
} digit_interpretation_definition_request_t;

typedef struct
{
    unsigned int established;
    char meaning[DIGIT_INTERPRETATION_TERM_MAX];
} digit_interpretation_definition_result_t;


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
    unsigned int lookup_count;
    digit_interpretation_lookup_t lookups[DIGIT_INTERPRETATION_TRACE_MAX];
} digit_interpretation_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
