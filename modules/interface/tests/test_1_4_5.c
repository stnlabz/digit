#include <stdio.h>
#include <string.h>
#include "knowledge_query.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.5: the same
 * canonical ID rule governs bulk and exact retrieval. */
static unsigned int count,failed;
static void check(int ok,const char *name){
 ++count;printf("%s 1.4.5 %02u - %s\n",ok?"PASS":"FAIL",count,name);
 if(!ok)++failed;
}
int main(void){
 digit_knowledge_record_t r[2]={{0}};
 char json[12000];
 strcpy(r[0].id,"source_123-A");
 strcpy(r[0].source,"manual:one");
 strcpy(r[0].category,"engineering");
 strcpy(r[0].text,"Evidence one");
 r[1]=r[0];strcpy(r[1].id,"source_124-B");
 check(digit_knowledge_results_valid(r,2),"valid bulk identities accepted");
 check(digit_knowledge_result_json(r,2,json,sizeof(json)) &&
       strstr(json,"source_123-A")!=NULL,"canonical IDs preserved in JSON");
 strcpy(r[1].id,"path/escape");
 check(digit_knowledge_record_valid(&r[1]),"record remains valid UTF-8");
 check(!digit_knowledge_results_valid(r,2),"slash in bulk record ID rejected");
 check(!digit_knowledge_result_json(r,2,json,sizeof(json)) && json[0]==0,
       "unsafe bulk ID produces no JSON");
 strcpy(r[1].id,"query?injection");
 check(!digit_knowledge_results_valid(r,2),"query delimiter rejected");
 strcpy(r[1].id,"dot.segment");
 check(!digit_knowledge_results_valid(r,2),"dot-containing ID rejected");
 strcpy(r[1].id,"space separated");
 check(!digit_knowledge_results_valid(r,2),"space-containing ID rejected");
 strcpy(r[1].id,"colon:value");
 check(!digit_knowledge_results_valid(r,2),"colon-containing ID rejected");
 strcpy(r[1].id,"percent%2F");
 check(!digit_knowledge_results_valid(r,2),"encoded path delimiter rejected");
 strcpy(r[1].id,"#fragment");
 check(!digit_knowledge_results_valid(r,2),"fragment identifier rejected");
 strcpy(r[1].id,"source_123-A");
 check(!digit_knowledge_results_valid(r,2),"identical record ID still rejected");
 strcpy(r[1].id,"source_124-B");
 r[1].source[0]=0;
 check(!digit_knowledge_results_valid(r,2),"missing source still rejected");
 r[1]=r[0];strcpy(r[1].id,"source_124-B");
 check(digit_knowledge_results_valid(r,2),"valid record accepted after denial");
 check(digit_knowledge_result_json(r,2,json,sizeof(json)) &&
       strstr(json,"\"evidence_count\":2")!=NULL,
       "valid bulk results recover after denials");
 check(digit_knowledge_results_valid(NULL,0),"empty result set remains valid");
 check(digit_knowledge_result_json(NULL,0,json,sizeof(json)) &&
       strstr(json,"\"answer\":\"UNKNOWN\"")!=NULL,
       "no-match result remains UNKNOWN");
 printf("Interface 1.4.5 milestone: %u executed, %u failed\n",count,failed);
 return failed!=0;
}
