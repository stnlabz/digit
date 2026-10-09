#include <string.h>
#include "corpus_response.h"
/* [AI:GPT-6 | 2026-10-08] 1.5.0: validate fixed-field bounds before comparing Core record identities. */
int digit_interface_corpus_exact_valid(const digit_knowledge_record_t *record,int found,const char *requested_id)
{
 if(!digit_knowledge_record_id_valid(requested_id)||(found!=0&&found!=1))return 0;
 if(!found)return 1;
 return record && digit_knowledge_record_valid(record) &&
        digit_knowledge_record_id_valid(record->id) &&
        strcmp(record->id,requested_id)==0;
}
int digit_interface_corpus_search_valid(const digit_knowledge_record_t *records,size_t count,size_t capacity)
{
 if(count>capacity || count>DIGIT_KNOWLEDGE_RECORD_MAX)return 0;
 return digit_knowledge_results_valid(records,count);
}
