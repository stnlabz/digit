#include <stdio.h>
#include <string.h>
#include "grant_request.h"
/* [AI:GPT-6 | 2026-10-09] 1.6.0 positive and negative input tests. */
static unsigned tests,failures;
static void check(int valid,const char *label){
 ++tests;
 if(valid)printf("PASS %02u - %s\n",tests,label);
 else{++failures;printf("FAIL %02u - %s\n",tests,label);}
}
int main(void){
 digit_grant_request_t r;
 char long_name[200];
 memset(long_name,'x',sizeof(long_name)-1);
 long_name[sizeof(long_name)-1]=0;
 check(digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei",&r),"valid grant parsed");
 check(strcmp(r.organization,"stn-labz")==0,"organization intact");
 check(strcmp(r.project,"operations")==0,"project intact");
 check(strcmp(r.channel,"channel-01")==0,"channel intact");
 check(strcmp(r.user,"poemei")==0,"operator intact");
 check(digit_grant_request_parse("team-chaos\tsecurity\tchannel-02\tpoemei",&r),"separate organization syntax");
 check(!digit_grant_request_parse(NULL,&r),"null input denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei",NULL),"null output denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01",&r),"missing user denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei\textra",&r),"extra field denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\t",&r),"empty user denied");
 check(!digit_grant_request_parse("\toperations\tchannel-01\tpoemei",&r),"empty org denied");
 check(!digit_grant_request_parse("../other\toperations\tchannel-01\tpoemei",&r),"traversal denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei\n",&r),"newline denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tbad user",&r),"spaces denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tbad\ruser",&r),"carriage return denied");
 check(!digit_grant_request_parse(long_name,&r),"oversize denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei\t",&r),"trailing tab denied");
 printf("Interface 1.6.0 scope parser: %u tests, %u failed\n",tests,failures);
 return failures?1:0;
}
