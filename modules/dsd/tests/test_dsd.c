#include <stdio.h>
#include <string.h>
#include "dsd.h"
static int failures;
static void check(int condition,const char *name){
    printf("%s %s\n",condition?"PASS":"FAIL",name);
    if(!condition)failures++;
}
int main(void){
    stnlabz_module_qualification_result_t result={0};
    const stnlabz_module_descriptor_t *module=stnlabz_module_get_descriptor();
    check(module!=NULL,"DSD descriptor");
    check(module&&strcmp(module->id,"dsd")==0,"DSD module identity");
    check(module&&module->version_major==1&&module->version_minor==2&&
          module->version_patch==0,"DSD 1.2.0 version");
    check(module&&module->qualify&&module->qualify(&result)==STNLABZ_MODULE_OK,
          "executable qualification");
    check(result.tests_executed==15&&result.tests_passed==15&&
          result.tests_failed==0,"all diagnostic cases measured");
    check(result.negative_test_executed==3&&result.negative_test_passed==3,
          "negative tests measured");
    printf("DSD tests: %d failure(s)\n",failures);
    return failures?1:0;
}
