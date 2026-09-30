#ifndef DIGIT_CORPUS_H
#define DIGIT_CORPUS_H

#include <stddef.h>
#include "module.h"

#define DIGIT_CORPUS_RECORD_ID_MAX 65
#define DIGIT_CORPUS_CATEGORY_MAX 64
#define DIGIT_CORPUS_SOURCE_MAX 256
#define DIGIT_CORPUS_TEXT_MAX 4096

typedef struct
{
    char id[DIGIT_CORPUS_RECORD_ID_MAX];
    char category[DIGIT_CORPUS_CATEGORY_MAX];
    char source[DIGIT_CORPUS_SOURCE_MAX];
    char text[DIGIT_CORPUS_TEXT_MAX];
} digit_corpus_record_t;

int digit_corpus_validate(const digit_corpus_record_t *record);
int digit_corpus_append(const char *path, const digit_corpus_record_t *record);
int digit_corpus_contains(const char *path, const char *record_id);
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
