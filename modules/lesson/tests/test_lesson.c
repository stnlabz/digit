#include <stdio.h>
#include <string.h>

#include "lesson.h"

static unsigned int executed=0,failed=0;
static void check(int condition,const char *name){++executed;if(condition)printf("PASS %02u - %s\n",executed,name);else{++failed;printf("FAIL %02u - %s\n",executed,name);}}

int main(void)
{
    const stnlabz_module_descriptor_t *descriptor=stnlabz_module_get_descriptor();
    stnlabz_module_qualification_result_t qualification;
    digit_lesson_ingest_request_t request;
    digit_lesson_ingest_result_t result;
    memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));
    check(descriptor!=NULL,"descriptor is exported");
    check(descriptor!=NULL&&strcmp(descriptor->id,"lesson")==0,"module identity is lesson");
    check(descriptor!=NULL&&descriptor->version_major==1&&descriptor->version_minor==1&&descriptor->version_patch==0,"internal version is 1.1.0");
    check(descriptor!=NULL&&descriptor->qualify!=NULL,"qualification callback exists");
    check(descriptor!=NULL&&descriptor->qualify(&qualification)==STNLABZ_MODULE_OK,"qualification executes");
    check(qualification.tests_executed>=STNLABZ_MODULE_MIN_TESTS,"required test count is reported");
    check(qualification.tests_passed==qualification.tests_executed&&qualification.tests_failed==0,"required tests pass");
    check(qualification.negative_test_executed&&qualification.negative_test_passed,"negative validation passes");
    check(sizeof(request.name)==DIGIT_LESSON_NAME_MAX,"lesson name contract is bounded");
    check(sizeof(result.path)==DIGIT_LESSON_PATH_MAX,"lesson path contract is bounded");
    printf("\nLesson module tests: %u executed, %u failed\n",executed,failed);
    return failed==0?0:1;
}
