#include <stdio.h>
#include <string.h>
#include "knowledge_query.h"

/* [AI:GPT-6 | 2026-10-08] Interface 1.4.2: verify complete,
 * unambiguous evidence sets before returning source-attributed JSON. */
static unsigned int executed,failed;
static void check(int ok,const char *label)
{
    ++executed;
    printf("%s 1.4.2 %02u - %s\n",ok?"PASS":"FAIL",executed,label);
    if(!ok)++failed;
}
static digit_knowledge_record_t record(const char *id,const char *source)
{
    digit_knowledge_record_t r={0};
    (void)snprintf(r.id,sizeof(r.id),"%s",id);
    (void)snprintf(r.category,sizeof(r.category),"%s","engineering");
    (void)snprintf(r.source,sizeof(r.source),"%s",source);
    (void)snprintf(r.text,sizeof(r.text),"%s","Verified evidence");
    return r;
}
int main(void)
{
    digit_knowledge_record_t r[2];
    char json[12000],small[8];
    r[0]=record("record-1","manual:1");
    r[1]=record("record-2","manual:2");
    check(digit_knowledge_results_valid(NULL,0),"empty result set valid");
    check(digit_knowledge_results_valid(r,1),"one sourced record valid");
    check(digit_knowledge_results_valid(r,2),"distinct sourced records valid");
    check(digit_knowledge_result_json(r,2,json,sizeof(json)) &&
          strstr(json,"\"evidence_count\":2") &&
          strstr(json,"\"source\":\"manual:1\"") &&
          strstr(json,"\"source\":\"manual:2\""),
          "distinct evidence preserved in output");
    check(!digit_knowledge_results_valid(NULL,1),
          "missing nonempty record set rejected");
    check(!digit_knowledge_results_valid(r,DIGIT_KNOWLEDGE_RECORD_MAX+1),
          "count greater than maximum rejected");
    r[1].id[0]=0;
    check(!digit_knowledge_results_valid(r,2),
          "empty second record ID rejected");
    check(!digit_knowledge_result_json(r,2,json,sizeof(json)) && json[0]==0,
          "invalid second record fails closed before output");
    r[1]=record("record-1","manual:2");
    check(!digit_knowledge_results_valid(r,2),
          "duplicate ID with different source rejected");
    check(!digit_knowledge_result_json(r,2,json,sizeof(json)) && json[0]==0,
          "conflicting evidence IDs produce no partial result");
    r[1]=record("record-1","manual:1");
    check(!digit_knowledge_results_valid(r,2),
          "duplicate ID with identical source rejected");
    r[1]=record("record-2","manual:2");
    r[1].source[0]=0;
    check(!digit_knowledge_results_valid(r,2),
          "record missing provenance rejected");
    r[1]=record("record-2","manual:2");
    r[1].text[0]=0;
    check(!digit_knowledge_results_valid(r,2),
          "record missing evidence text rejected");
    r[1]=record("record-2","manual:2");
    check(digit_knowledge_result_json(NULL,0,json,sizeof(json)) &&
          strstr(json,"\"answer\":\"UNKNOWN\""),
          "zero evidence retains explicit unknown response");
    check(!digit_knowledge_result_json(r,2,small,sizeof(small)) && small[0]==0,
          "insufficient output buffer rejected");
    check(!digit_knowledge_result_json(r,2,NULL,0),
          "null output buffer rejected");
    check(digit_knowledge_result_json(r,2,json,sizeof(json)) &&
          strstr(json,"\"answer\":\"EVIDENCE_ONLY\""),
          "valid result set recovers after rejected input");
    printf("Interface 1.4.2 milestone: %u executed, %u failed\n",executed,failed);
    return failed!=0;
}
