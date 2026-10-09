#include <stdio.h>
#include <string.h>
#include "dispatcher.h"

/* [AI:GPT-6 | 2026-10-09] Run executable module qualification and
 * validate the returned evidence rather than assuming declared passes. */
int main(void){
 const stnlabz_module_descriptor_t *d=stnlabz_module_get_descriptor();
 stnlabz_module_qualification_result_t q;
 if(!d||strcmp(d->id,"dispatcher")||!d->qualify){
  fprintf(stderr,"Dispatcher descriptor invalid\\n");return 1;
 }
 memset(&q,0,sizeof(q));
 if(d->qualify(&q)!=STNLABZ_MODULE_OK||q.tests_executed<10||
    q.tests_passed!=q.tests_executed||q.tests_failed!=0||
    q.negative_test_executed<1||q.negative_test_passed!=q.negative_test_executed){
  fprintf(stderr,"Dispatcher qualification FAILED (%u/%u, negative %d/%d)\\n",
    q.tests_passed,q.tests_executed,q.negative_test_passed,q.negative_test_executed);
  return 1;
 }
 printf("Dispatcher qualification: %u/%u checks, negative %d/%d — PASS\\n",
    q.tests_passed,q.tests_executed,q.negative_test_passed,q.negative_test_executed);
 return 0;
}
