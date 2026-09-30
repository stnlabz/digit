#ifndef DIGIT_CORPUS_BUILDER_H
#define DIGIT_CORPUS_BUILDER_H

#include <stddef.h>
#include "module.h"

#define DIGIT_CORPUS_BUILDER_SERVICE "corpus_builder.evaluate"
#define DIGIT_CORPUS_BUILDER_TEXT_MAX 4096
#define DIGIT_CORPUS_BUILDER_CATEGORY_MAX 64
#define DIGIT_CORPUS_BUILDER_REASON_MAX 256

typedef struct
{
    char text[DIGIT_CORPUS_BUILDER_TEXT_MAX];
    char source[256];
} digit_corpus_builder_request_t;

typedef struct
{
    int candidate;
    unsigned int confidence;
    char category[DIGIT_CORPUS_BUILDER_CATEGORY_MAX];
    char reason[DIGIT_CORPUS_BUILDER_REASON_MAX];
} digit_corpus_builder_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
