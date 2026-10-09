#include <stdio.h>
#include <string.h>
#include "knowledge_query.h"

/* [AI:GPT-6 | 2026-10-08] 1.4.3 record identity,
 * provenance and explicit unknown handling. */
static unsigned int total,failed;
static void check(int ok,const char *label)
{
    ++total;
    printf("%s 1.4.3 %02u - %s\n",ok?"PASS":"FAIL",total,label);
    if(!ok)++failed;
}
int main(void)
{
    digit_knowledge_record_t record={0};
    char response[12000],tiny[8],longid[66];
    strcpy(record.id,"record-001");
    strcpy(record.category,"engineering");
    strcpy(record.source,"manual:section-1");
    strcpy(record.text,"Verified evidence");
    check(digit_knowledge_record_id_valid("record-001"),
          "valid record identifier accepted");
    check(digit_knowledge_record_id_valid("ABCD_123"),
          "bounded alternate identifier accepted");
    check(!digit_knowledge_record_id_valid(NULL),"null identifier denied");
    check(!digit_knowledge_record_id_valid(""),"empty identifier denied");
    check(!digit_knowledge_record_id_valid("record/other"),
          "path delimiter denied");
    check(!digit_knowledge_record_id_valid("record?x=1"),
          "query string delimiter denied");
    check(!digit_knowledge_record_id_valid("record\nX"),
          "header injection denied");
    memset(longid,'a',sizeof(longid));
    longid[65]=0;
    check(!digit_knowledge_record_id_valid(longid),
          "oversized identifier denied");
    check(digit_knowledge_single_json(&record,1,response,sizeof(response)) &&
          strstr(response,"\"answer\":\"EVIDENCE_ONLY\"") &&
          strstr(response,"\"source\":\"manual:section-1\"") &&
          strstr(response,"\"id\":\"record-001\""),
          "valid lookup preserves identity and source");
    check(digit_knowledge_single_json(NULL,0,response,sizeof(response)) &&
          strstr(response,"\"answer\":\"UNKNOWN\"") &&
          strstr(response,"\"evidence_count\":0"),
          "missing record reports explicit unknown");
    check(!digit_knowledge_single_json(NULL,1,response,sizeof(response)) &&
          response[0]==0,"found result without record denied");
    check(!digit_knowledge_single_json(&record,2,response,sizeof(response)) &&
          response[0]==0,"invalid found flag denied");
    check(!digit_knowledge_single_json(&record,-1,response,sizeof(response)),
          "negative found flag denied");
    record.source[0]=0;
    check(!digit_knowledge_single_json(&record,1,response,sizeof(response)) &&
          response[0]==0,"unsourced record denied");
    strcpy(record.source,"manual:section-1");
    strcpy(record.id,"invalid/id");
    check(!digit_knowledge_single_json(&record,1,response,sizeof(response)),
          "unsafe returned record identity denied");
    strcpy(record.id,"record-001");
    check(!digit_knowledge_single_json(&record,1,tiny,sizeof(tiny)) &&
          tiny[0]==0,"short response capacity fails closed");
    check(!digit_knowledge_single_json(&record,1,NULL,0),
          "null response buffer denied");
    check(digit_knowledge_single_json(&record,1,response,sizeof(response)) &&
          strstr(response,"\"evidence_count\":1"),
          "valid lookup recovers after negative cases");
    printf("Interface 1.4.3 milestone: %u executed, %u failed\n",total,failed);
    return failed!=0;
}
