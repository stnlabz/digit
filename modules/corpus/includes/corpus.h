#ifndef DIGIT_CORPUS_H
#define DIGIT_CORPUS_H

#include <stddef.h>
#include "module.h"

#define DIGIT_CORPUS_RECORD_ID_MAX 65
#define DIGIT_CORPUS_CATEGORY_MAX 64
#define DIGIT_CORPUS_SOURCE_MAX 256
#define DIGIT_CORPUS_TEXT_MAX 4096
#define DIGIT_CORPUS_SEARCH_MAX 16
#define DIGIT_CORPUS_PATH "/opt/digit/corpus/corpus.tsv"
#define DIGIT_CORPUS_CONTAINS_SERVICE "corpus.contains"
#define DIGIT_CORPUS_APPEND_SERVICE "corpus.append"
#define DIGIT_CORPUS_GET_SERVICE "corpus.get"
#define DIGIT_CORPUS_SEARCH_SERVICE "corpus.search"

typedef struct
{
    char id[DIGIT_CORPUS_RECORD_ID_MAX];
    char category[DIGIT_CORPUS_CATEGORY_MAX];
    char source[DIGIT_CORPUS_SOURCE_MAX];
    char text[DIGIT_CORPUS_TEXT_MAX];
} digit_corpus_record_t;

typedef struct { int contains; } digit_corpus_contains_result_t;
typedef struct { int appended; } digit_corpus_append_result_t;

typedef struct
{
    int found;
    digit_corpus_record_t record;
} digit_corpus_get_result_t;

typedef struct
{
    char query[DIGIT_CORPUS_TEXT_MAX];
} digit_corpus_search_request_t;

typedef struct
{
    size_t count;
    digit_corpus_record_t records[DIGIT_CORPUS_SEARCH_MAX];
} digit_corpus_search_result_t;

int digit_corpus_validate(const digit_corpus_record_t *record);
int digit_corpus_append(const char *path, const digit_corpus_record_t *record);
int digit_corpus_contains(const char *path, const char *record_id);
int digit_corpus_get(const char *path, const char *record_id, digit_corpus_record_t *record);
size_t digit_corpus_search(const char *path, const char *query, digit_corpus_record_t *records, size_t capacity);
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
