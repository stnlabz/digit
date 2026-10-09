#ifndef DIGIT_INTERFACE_CORPUS_RESPONSE_H
#define DIGIT_INTERFACE_CORPUS_RESPONSE_H
#include <stddef.h>
#include "knowledge_query.h"
/* [AI:GPT-6 | 2026-10-08] 1.5.0 Core Corpus boundary. */
int digit_interface_corpus_exact_valid(const digit_knowledge_record_t *record,
 int found,const char *requested_id);
int digit_interface_corpus_search_valid(const digit_knowledge_record_t *records,
 size_t count,size_t capacity);
#endif
