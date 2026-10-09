#include <stdio.h>
#include <string.h>
#include "corpus_response.h"
/* [AI:GPT-6 | 2026-10-08] 1.5.2 shared-route Corpus response parity. */
static unsigned int executed,failed;
static void check(int ok,const char *name){
 ++executed;
 printf("%s 1.5.2 %02u - %s\n",ok?"PASS":"FAIL",executed,name);
 if(!ok)++failed;
}
static digit_knowledge_record_t record(void){
 digit_knowledge_record_t r={0};
 strcpy(r.id,"record-1");strcpy(r.category,"engineering");
 strcpy(r.source,"manual:1");strcpy(r.text,"observed evidence");
 return r;
}
int main(void){
 digit_knowledge_record_t records[2];char exact[10000],bulk[10000];
 records[0]=record();records[1]=record();strcpy(records[1].id,"record-2");
 check(digit_interface_corpus_exact_json(&records[0],1,"record-1",exact,sizeof(exact)),"exact evidence rendered");
 check(digit_interface_corpus_search_json(records,1,2,bulk,sizeof(bulk)),"one-record search rendered");
 check(strcmp(exact,bulk)==0,"exact and search evidence JSON match");
 check(digit_interface_corpus_exact_json(NULL,0,"record-1",exact,sizeof(exact)),"missing exact rendered");
 check(digit_interface_corpus_search_json(NULL,0,2,bulk,sizeof(bulk)),"empty search rendered");
 check(strcmp(exact,bulk)==0,"UNKNOWN exact and empty search JSON match");
 check(digit_interface_corpus_search_json(records,2,2,bulk,sizeof(bulk)),"two records rendered");
 check(strstr(bulk,"record-2")!=NULL,"second record preserved");
 check(!digit_interface_corpus_search_json(records,3,2,bulk,sizeof(bulk)),"oversized count rejected");
 check(bulk[0]==0,"oversized request clears output");
 check(!digit_interface_corpus_exact_json(&records[0],1,"wrong-id",exact,sizeof(exact)),"wrong identity rejected");
 check(exact[0]==0,"wrong identity leaves no JSON");
 records[1].source[0]=0;
 check(!digit_interface_corpus_search_json(records,2,2,bulk,sizeof(bulk)),"missing provenance rejected");
 check(bulk[0]==0,"invalid provenance clears output");
 strcpy(records[1].source,"manual:2");strcpy(records[1].id,"record-1");
 check(!digit_interface_corpus_search_json(records,2,2,bulk,sizeof(bulk)),"duplicate evidence ID rejected");
 strcpy(records[1].id,"record-2");
 check(digit_interface_corpus_search_json(records,2,2,bulk,sizeof(bulk)),"valid response recovered");
 check(!digit_interface_corpus_exact_json(&records[0],1,"record-1",exact,1),"truncated exact JSON rejected");
 check(!digit_interface_corpus_search_json(records,2,2,bulk,1),"truncated search JSON rejected");
 printf("Interface 1.5.2 milestone: %u executed, %u failed\n",executed,failed);
 return failed!=0;
}
