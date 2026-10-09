#include <stdio.h>
#include <string.h>
#include "interpretation.h"

/* [AI:GPT-6 | 2026-10-09] Exercise actual measured Interpretation
 * qualification; runtime Corpus integration is a separate acceptance gate. */
int main(void){
 const stnlabz_module_descriptor_t *d=stnlabz_module_get_descriptor();
 stnlabz_module_qualification_result_t q;
 if(!d||strcmp(d->id,"interpretation")||!d->qualify||
    d->version_major!=1||d->version_minor!=0||d->version_patch!=5)return 1;
 if(d->qualify(NULL)!=STNLABZ_MODULE_ERR_INVALID_ARGUMENT)return 1;
 memset(&q,0,sizeof(q));
 if(d->qualify(&q)!=STNLABZ_MODULE_OK||q.tests_executed<10||
    q.tests_passed!=q.tests_executed||q.tests_failed||
    !q.negative_test_executed||q.negative_test_passed!=q.negative_test_executed){
  fprintf(stderr,"Interpretation qualification FAILED: %u/%u, negative %d/%d\n",
   q.tests_passed,q.tests_executed,q.negative_test_passed,q.negative_test_executed);
  return 1;
 }
 printf("Interpretation qualification: %u/%u checks, negative %d/%d - PASS\n",
  q.tests_passed,q.tests_executed,q.negative_test_passed,q.negative_test_executed);
 return 0;
}
