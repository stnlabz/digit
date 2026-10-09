#include <stdio.h>
#include <string.h>
#include "validator.h"

/* [AI:GPT-6 | 2026-10-09] Execute actual qualification, verify counters
 * and exercise invalid qualification arguments. */
int main(void){
 const stnlabz_module_descriptor_t *descriptor=stnlabz_module_get_descriptor();
 stnlabz_module_qualification_result_t result;
 if(!descriptor||strcmp(descriptor->id,"validator")||!descriptor->qualify)
  return 1;
 if(descriptor->qualify(NULL)!=STNLABZ_MODULE_ERR_INVALID_ARGUMENT)
  return 1;
 memset(&result,0,sizeof(result));
 if(descriptor->version_major!=1||descriptor->version_minor!=0||
    descriptor->version_patch!=5||descriptor->qualify(&result)!=STNLABZ_MODULE_OK||
    result.tests_executed<10||result.tests_passed!=result.tests_executed||
    result.tests_failed||!result.negative_test_executed||
    result.negative_test_passed!=result.negative_test_executed){
  fprintf(stderr,"Validator qualification FAILED: %u/%u, negative %d/%d\n",
    result.tests_passed,result.tests_executed,
    result.negative_test_passed,result.negative_test_executed);
  return 1;
 }
 printf("Validator qualification: %u/%u checks, negative %d/%d - PASS\n",
   result.tests_passed,result.tests_executed,
   result.negative_test_passed,result.negative_test_executed);
 return 0;
}
