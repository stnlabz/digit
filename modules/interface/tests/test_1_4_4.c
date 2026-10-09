#include <stdio.h>
#include <string.h>
#include "knowledge_query.h"

/* [AI:GPT-6 | 2026-10-08] Interface 1.4.4:
 * canonical UTF-8 ingress and provenance-safe JSON output. */
static unsigned int total,failed;
static void check(int good,const char *label)
{
    ++total;
    printf("%s 1.4.4 %02u - %s\n",good?"PASS":"FAIL",total,label);
    if(!good)++failed;
}
static digit_knowledge_record_t record(void)
{
    digit_knowledge_record_t r={0};
    strcpy(r.id,"record-001");
    strcpy(r.category,"engineering");
    strcpy(r.source,"manual:section-1");
    strcpy(r.text,"Verified source");
    return r;
}
int main(void)
{
    char output[12000];
    digit_knowledge_record_t r=record();
    const char *valid_2="caf\303\251";
    const char *valid_3="\342\202\254";
    const char *valid_4="\360\237\222\234";
    const char *bad_lone="\200";
    const char *bad_cont="\303X";
    const char *bad_overlong="\300\257";
    const char *bad_overlong3="\340\200\200";
    const char *bad_surrogate="\355\240\200";
    const char *bad_overmax="\364\220\200\200";
    const char *bad_trunc="\360\237\222";
    check(digit_knowledge_query_valid("plain ASCII"),
          "ASCII knowledge query accepted");
    check(digit_knowledge_query_valid(valid_2),
          "valid two-byte UTF-8 query accepted");
    check(digit_knowledge_query_valid(valid_3),
          "valid three-byte UTF-8 query accepted");
    check(digit_knowledge_query_valid(valid_4),
          "valid four-byte UTF-8 query accepted");
    check(!digit_knowledge_query_valid(bad_lone),
          "orphan continuation rejected");
    check(!digit_knowledge_query_valid(bad_cont),
          "missing continuation rejected");
    check(!digit_knowledge_query_valid(bad_overlong),
          "overlong two-byte encoding rejected");
    check(!digit_knowledge_query_valid(bad_overlong3),
          "overlong three-byte encoding rejected");
    check(!digit_knowledge_query_valid(bad_surrogate),
          "Unicode surrogate rejected");
    check(!digit_knowledge_query_valid(bad_overmax),
          "Unicode out-of-range scalar rejected");
    check(!digit_knowledge_query_valid(bad_trunc),
          "truncated sequence rejected");
    check(!digit_knowledge_query_valid("bad\nline"),
          "ASCII control characters still rejected");
    strcpy(r.text,valid_4);
    check(digit_knowledge_record_valid(&r),
          "valid Unicode evidence accepted");
    check(digit_knowledge_result_json(&r,1,output,sizeof(output)) &&
          strstr(output,valid_4)!=NULL,
          "valid UTF-8 evidence retained in JSON");
    strcpy(r.text,bad_lone);
    check(!digit_knowledge_record_valid(&r),
          "malformed UTF-8 evidence rejected");
    check(!digit_knowledge_result_json(&r,1,output,sizeof(output)) &&
          output[0]==0,
          "malformed evidence fails closed without partial JSON");
    r=record();strcpy(r.source,bad_overlong);
    check(!digit_knowledge_result_json(&r,1,output,sizeof(output)) &&
          output[0]==0,
          "malformed provenance rejected");
    r=record();strcpy(r.category,bad_surrogate);
    check(!digit_knowledge_record_valid(&r),
          "malformed category rejected");
    r=record();strcpy(r.id,bad_cont);
    check(!digit_knowledge_record_valid(&r),
          "malformed identifier rejected");
    r=record();
    check(digit_knowledge_result_json(NULL,0,output,sizeof(output)) &&
          strstr(output,"\"answer\":\"UNKNOWN\"")!=NULL,
          "empty evidence retains UNKNOWN");
    check(digit_knowledge_result_json(&r,1,output,sizeof(output)) &&
          strstr(output,"\"source\":\"manual:section-1\"")!=NULL,
          "valid evidence recovers after malformed input");
    printf("Interface 1.4.4 milestone: %u executed, %u failed\n",total,failed);
    return failed!=0;
}
