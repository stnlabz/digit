#include <stdio.h>
#include <string.h>
#include "dashboard_response.h"
/* [AI:GPT-6 | 2026-10-08] Interface 1.5.6 dashboard serialization
 * milestone: positive, negative, denied, and boundary assertions. */
static unsigned executed,failed;
static void check(int value,const char *label){
    ++executed;
    if(value)printf("PASS %02u - %s\n",executed,label);
    else{++failed;printf("FAIL %02u - %s\n",executed,label);}
}
int main(void){
    char out[256];
    check(digit_dashboard_response(1,1,0,0,128,256,out,sizeof(out)),"authorized snapshot accepted");
    check(strcmp(out,"{\"authorized\":true,\"scope\":\"digit-operations-read\",\"channels\":1,\"alerts\":0,\"unacknowledged_alerts\":0}\n")==0,"exact read-only schema");
    check(!digit_dashboard_response(0,1,0,0,128,256,out,sizeof(out)),"unauthorized request denied");
    check(out[0]==0,"denied request does not disclose data");
    check(!digit_dashboard_response(2,1,0,0,128,256,out,sizeof(out)),"noncanonical authorization rejected");
    check(!digit_dashboard_response(1,129,0,0,128,256,out,sizeof(out)),"channel count above limit denied");
    check(!digit_dashboard_response(1,1,257,0,128,256,out,sizeof(out)),"alert count above limit denied");
    check(!digit_dashboard_response(1,1,2,3,128,256,out,sizeof(out)),"unacknowledged above total denied");
    check(digit_dashboard_response(1,128,256,256,128,256,out,sizeof(out)),"boundary counts accepted");
    check(strstr(out,"\"channels\":128")!=NULL && strstr(out,"\"unacknowledged_alerts\":256")!=NULL,"boundary counts serialized");
    check(!digit_dashboard_response(1,1,1,0,128,256,out,8),"truncated response denied");
    check(out[0]==0,"truncated response cleared");
    check(!digit_dashboard_response(1,1,1,0,128,256,NULL,sizeof(out)),"null output rejected");
    check(!digit_dashboard_response(1,1,1,0,128,256,out,0),"zero capacity rejected");
    printf("Interface 1.5.6 milestone: %u executed, %u failed\n",executed,failed);
    return failed?1:0;
}
