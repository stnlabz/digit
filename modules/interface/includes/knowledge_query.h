#ifndef DIGIT_KNOWLEDGE_QUERY_H
#define DIGIT_KNOWLEDGE_QUERY_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-08] Interface 1.5.0.
 * Local bounded knowledge query and source-preserving record validation. */
#define DIGIT_KNOWLEDGE_QUERY_MAX 4096u
#define DIGIT_KNOWLEDGE_RECORD_MAX 16u
typedef struct {
    char id[65];
    char category[64];
    char source[256];
    char text[4096];
} digit_knowledge_record_t;
int digit_knowledge_query_valid(const char *query);
int digit_knowledge_record_valid(const digit_knowledge_record_t *record);
int digit_knowledge_result_json(const digit_knowledge_record_t *records,
                               size_t count,char *output,size_t capacity);
#endif
