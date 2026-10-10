#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include "../src/stn2.c"
int main(void){
 stnlabz_module_qualification_result_t q={0};
 digit_stn2_request_t in={0};digit_stn2_result_t out={0};size_t used=0;
 assert(qualify(&q)==STNLABZ_MODULE_OK&&q.tests_passed==10);
 assert(execute(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
 assert(!authorized_command("gen intel admin"));
 assert(!authorized_command("weekly brief"));
 puts("STN-2 offline qualification passed");return 0;
}
