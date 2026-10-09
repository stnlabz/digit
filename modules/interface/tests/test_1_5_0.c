#include <stdio.h>
#include <string.h>
#include "corpus_response.h"
/* [AI:GPT-6 | 2026-10-08] 1.5.0 deterministic Corpus response admission tests. */
static unsigned int executed,failed;
static void check(int ok,const char *name){
 ++executed;
 printf("%s 1.5.0 %02u - %s\n",ok?"PASS":"FAIL",executed,name);
 if(!ok)++failed;
}
static digit_knowledge_record_t example(void){
 digit_knowledge_record_t r={0};
 strcpy(r.id,"record-1");strcpy(r.category,"engineering");
 strcpy(r.source,"manual:1");strcpy(r.text,"Evidence");
 return r;
}
int main(void){
 digit_knowledge_record_t records[2]={0};
 records[0]=example();records[1]=example();
 strcpy(records[1].id,"record-2");
 check(digit_interface_corpus_exact_valid(&records[0],1,"record-1"),"matching exact record accepted");
 check(digit_interface_corpus_exact_valid(NULL,0,"record-1"),"explicit missing record accepted");
 check(!digit_interface_corpus_exact_valid(NULL,1,"record-1"),"missing payload rejected");
 check(!digit_interface_corpus_exact_valid(&records[0],2,"record-1"),"invalid found flag rejected");
 check(!digit_interface_corpus_exact_valid(&records[0],-1,"record-1"),"negative found flag rejected");
 check(!digit_interface_corpus_exact_valid(&records[0],1,"record-2"),"identity mismatch rejected");
 check(!digit_interface_corpus_exact_valid(&records[0],1,"../bad"),"unsafe requested identifier rejected");
 check(!digit_interface_corpus_exact_valid(NULL,0,NULL),"null requested identifier rejected");
 check(!digit_interface_corpus_exact_valid(NULL,0,""),"empty requested identifier rejected");
 check(digit_interface_corpus_search_valid(records,2,2),"bounded sourced records accepted");
 check(digit_interface_corpus_search_valid(NULL,0,2),"empty evidence accepted as UNKNOWN");
 check(!digit_interface_corpus_search_valid(NULL,1,2),"missing nonempty result rejected");
 check(!digit_interface_corpus_search_valid(records,3,2),"oversized result count rejected before access");
 check(!digit_interface_corpus_search_valid(records,17,2),"knowledge maximum enforced");
 strcpy(records[1].id,"record-1");
 check(!digit_interface_corpus_search_valid(records,2,2),"duplicate evidence identity rejected");
 strcpy(records[1].id,"record-2");records[1].source[0]=0;
 check(!digit_interface_corpus_search_valid(records,2,2),"unsourced evidence rejected");
 strcpy(records[1].source,"manual:2");
 memset(records[1].id,'x',sizeof(records[1].id));
 check(!digit_interface_corpus_exact_valid(&records[1],1,"record-2"),"unterminated Core record ID rejected safely");
 strcpy(records[1].id,"record-2");
 memset(records[1].text,'x',sizeof(records[1].text));
 check(!digit_interface_corpus_search_valid(records,2,2),"unterminated evidence rejected");
 strcpy(records[1].text,"Evidence two");
 check(digit_interface_corpus_search_valid(records,2,2),"valid evidence recovers after malformed records");
 check(digit_interface_corpus_exact_valid(&records[1],1,"record-2"),"valid exact record recovers after malformed records");
 printf("Interface 1.5.0 milestone: %u executed, %u failed\n",executed,failed);
 return failed!=0;
}
