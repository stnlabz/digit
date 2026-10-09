#include <stdio.h>
#include <string.h>
#include "knowledge_query.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.1 knowledge-source and
 * unknown-response qualification coverage. */
static unsigned int count,failed;
static void check(int good,const char *name)
{
    ++count;
    printf("%s 1.4.1 %02u - %s\n",good?"PASS":"FAIL",count,name);
    if(!good)++failed;
}
int main(void)
{
    digit_knowledge_record_t r[2]={{0}};
    char result[12000],tiny[8],large[DIGIT_KNOWLEDGE_QUERY_MAX+1];
    strcpy(r[0].id,"record-001");
    strcpy(r[0].category,"engineering");
    strcpy(r[0].source,"manual:section-1");
    strcpy(r[0].text,"Deterministic behavior");
    r[1]=r[0];
    strcpy(r[1].id,"record-002");
    strcpy(r[1].source,"manual:section-2");
    strcpy(r[1].text,"Second observation");
    check(digit_knowledge_query_valid("What is deterministic?"),
          "bounded knowledge query accepted");
    check(!digit_knowledge_query_valid(NULL),"missing query denied");
    check(!digit_knowledge_query_valid(""),"empty query denied");
    check(!digit_knowledge_query_valid("bad\nquery"),"control character query denied");
    memset(large,'x',sizeof(large));large[sizeof(large)-1]=0;
    check(!digit_knowledge_query_valid(large),"oversized query denied");
    check(digit_knowledge_record_valid(&r[0]),"complete sourced record accepted");
    check(digit_knowledge_result_json(NULL,0,result,sizeof(result)) &&
          strstr(result,"\"answer\":\"UNKNOWN\"") &&
          strstr(result,"\"found\":false") &&
          strstr(result,"\"evidence_count\":0"),
          "no matches report explicit unknown");
    check(digit_knowledge_result_json(r,1,result,sizeof(result)) &&
          strstr(result,"\"answer\":\"EVIDENCE_ONLY\"") &&
          strstr(result,"\"source\":\"manual:section-1\""),
          "single result preserves provenance without generated answer");
    check(digit_knowledge_result_json(r,2,result,sizeof(result)) &&
          strstr(result,"\"evidence_count\":2") &&
          strstr(result,"record-002"),
          "multiple sourced records retain provenance");
    check(!digit_knowledge_result_json(r,17,result,sizeof(result)),
          "excessive result count denied");
    check(!digit_knowledge_result_json(NULL,1,result,sizeof(result)),
          "missing nonempty result array denied");
    check(!digit_knowledge_result_json(r,1,tiny,sizeof(tiny)) &&
          tiny[0]==0,"truncated JSON fails closed");
    check(!digit_knowledge_result_json(r,1,NULL,0),
          "missing output denied");
    r[0].source[0]=0;
    check(!digit_knowledge_record_valid(&r[0]),"missing provenance rejected");
    check(!digit_knowledge_result_json(r,1,result,sizeof(result)) &&
          result[0]==0,"unsourced result cannot be returned");
    strcpy(r[0].source,"manual:section-1");
    strcpy(r[0].text,"Quote: \"verified\" and \\ path");
    check(digit_knowledge_result_json(r,1,result,sizeof(result)) &&
          strstr(result,"\\\"verified\\\"") &&
          strstr(result,"\\\\ path"),
          "record quotes and slashes JSON escaped");
    strcpy(r[0].text,"bad\nrecord");
    check(!digit_knowledge_result_json(r,1,result,sizeof(result)),
          "control characters in evidence rejected");
    printf("Interface 1.4.1 milestone: %u executed, %u failed\n",count,failed);
    return failed!=0;
}
