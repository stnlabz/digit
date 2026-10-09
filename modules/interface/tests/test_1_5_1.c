#include <stdio.h>
#include <string.h>
#include "corpus_response.h"
/* [AI:GPT-6 | 2026-10-08] Interface 1.5.1 renderer checks. */
static unsigned int run,failed;
static void check(int ok,const char *name){++run;printf("%s 1.5.1 %02u - %s\n",ok?"PASS":"FAIL",run,name);if(!ok)++failed;}
int main(void){
 digit_knowledge_record_t a={0};char json[10000];
 strcpy(a.id,"record-1");strcpy(a.category,"engineering");strcpy(a.source,"manual:1");strcpy(a.text,"evidence");
 check(digit_interface_corpus_exact_json(&a,1,"record-1",json,sizeof(json)),"exact JSON accepted");
 check(strstr(json,"EVIDENCE_ONLY")!=NULL,"evidence-only response preserved");
 check(!digit_interface_corpus_exact_json(&a,1,"wrong",json,sizeof(json)),"wrong record denied");
 check(json[0]==0,"failed exact render clears output");
 check(digit_interface_corpus_exact_json(NULL,0,"record-1",json,sizeof(json)),"missing record JSON accepted");
 check(strstr(json,"UNKNOWN")!=NULL,"UNKNOWN response preserved");
 check(!digit_interface_corpus_exact_json(&a,1,"record-1",json,1),"truncated JSON denied");
 check(digit_interface_corpus_search_json(&a,1,1,json,sizeof(json)),"search JSON accepted");
 check(!digit_interface_corpus_search_json(&a,2,1,json,sizeof(json)),"oversize evidence denied");
 check(!digit_interface_corpus_search_json(NULL,1,1,json,sizeof(json)),"missing record array denied");
 check(digit_interface_corpus_search_json(NULL,0,1,json,sizeof(json)),"empty search rendered as UNKNOWN");
 check(!digit_interface_corpus_search_json(&a,1,1,json,1),"truncated search denied");
 printf("Interface 1.5.1 milestone: %u executed, %u failed\n",run,failed);
 return failed!=0;
}
