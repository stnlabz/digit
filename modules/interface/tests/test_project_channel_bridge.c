#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"
#include "project_channel_bridge.h"

/* [AI:GPT-6 | 2026-10-08] Mock Core service, no production registry writes. */
static int calls=0,fail_call=0;
static int mock(const char *service,const void *request,size_t request_size,
                void *response,size_t response_size,size_t *used,void *unused) {
    const digit_channel_create_request_t *in=request;
    digit_channel_create_response_t *out=response;
    (void)unused;
    ++calls;
    if(fail_call || strcmp(service,DIGIT_CHANNEL_SERVICE_CREATE)!=0 ||
       request_size!=sizeof(*in) || response_size<sizeof(*out) ||
       (strcmp(in->name,"security-stn-labz-second")!=0 &&
        strcmp(in->name,"security-stn-labz-third")!=0))return 0;
    memset(out,0,sizeof(*out));
    out->created=1;
    strcpy(out->channel.id,"channel-123-1");
    strcpy(out->channel.name,in->name);
    *used=sizeof(*out);
    return 1;
}
/* [AI:GPT-6 | 2026-10-08] Trusted Core host mock. */
static stnlabz_module_result_t mock_host_service(const char *service,
    const void *request,size_t request_size,void *response,
    size_t response_size,size_t *used)
{
    return mock(service,request,request_size,response,response_size,
                used,NULL)?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
}
static unsigned int count=0,failures=0;
static void check(int ok,const char *name) {
    ++count;
    if(ok)printf("PASS %02u - %s\n",count,name);
    else {++failures;printf("FAIL %02u - %s\n",count,name);}
}
int main(void) {
    char root[]="/tmp/digit-bridge-XXXXXX";
    char id[DIGIT_CHANNEL_ID_MAX],path[256],registry[256];
    FILE *sa_file;
    if(!mkdtemp(root))return 1;
    snprintf(registry,sizeof(registry),"%s/sa.tsv",root);
    sa_file=fopen(registry,"w");if(!sa_file)return 1;
    fputs("sysadmin\tstn-labz\tSA\t1\t1\t1\n",sa_file);
    fclose(sa_file);chmod(registry,0600);
    check(!digit_project_security_channel_id(root,"stn-labz","digit",id,sizeof(id)),"no unprovisioned binding");
    check(!digit_project_bind_security(root,"stn-labz","digit","sysadmin",registry,mock,NULL),"unprovisioned Core creation denied");
    check(!digit_project_bind_security(root,"stn-labz","digit","sysadmin",registry,NULL,NULL),"missing callback denied");
    check(digit_project_provision(root,"stn-labz","digit","poe","sysadmin",registry),"restricted project initialized");
    check(!digit_project_security_channel_id(root,"stn-labz","digit",id,sizeof(id)),"security binding not inferred");
    fail_call=1;
    check(!digit_project_bind_security(root,"stn-labz","digit","sysadmin",registry,mock,NULL),"Core service failure denied");
    check(!digit_project_security_channel_id(root,"stn-labz","digit",id,sizeof(id)),"failed reservation not published");
    check(!digit_project_bind_security(root,"stn-labz","digit","sysadmin",registry,mock,NULL),"failed reservation blocks duplicate creation");
    check(calls==1,"failed Core call not repeated");
    check(digit_project_provision(root,"stn-labz","second","poe","sysadmin",registry),"another project initialized");
    /* Successful Core reply must be persisted before the channel is visible. */
    snprintf(path,sizeof(path),"%s/stn-labz/second/security_channel.id",root);
    check(!digit_project_security_channel_id(root,"stn-labz","second",id,sizeof(id)),"second project starts without Core binding");
    fail_call=0;
    check(!digit_project_bind_security(root,"stn-labz","second","poe",registry,mock,NULL),"regular admin cannot invoke Core binding");
    check(!digit_project_bind_security(root,"stn-labz","second","missing",registry,mock,NULL),"unassigned identity cannot invoke Core binding");
    check(!digit_project_bind_security(root,"stn-labz","second","sysadmin",NULL,mock,NULL),"missing SA roster denies Core binding");
    check(calls==1,"denied actors never invoke Core");
    {
        char membership[300];
        FILE *m;
        snprintf(membership,sizeof(membership),"%s/stn-labz/second/security.tsv",root);
        m=fopen(membership,"w");if(!m)return 1;
        fputs("security\trestricted\tdigit\n",m);
        if(fclose(m)!=0)return 1;
        check(!digit_project_bind_security(root,"stn-labz","second","sysadmin",registry,mock,NULL),"SA without project security membership denied");
        m=fopen(membership,"w");if(!m)return 1;
        fputs("security\trestricted\tdigit\nsecurity\trestricted\tsysadmin\n",m);
        if(fclose(m)!=0)return 1;
        check(calls==1,"missing membership never invokes Core");
    }
    sa_file=fopen(registry,"w");if(!sa_file)return 1;
    fputs("sysadmin\tstn-labz\tSA\t1\t0\t1\n",sa_file);
    if(fclose(sa_file)!=0)return 1;
    check(!digit_project_bind_security(root,"stn-labz","second","sysadmin",registry,mock,NULL),"revoked SA cannot create Core channel");
    check(calls==1,"revoked SA cannot invoke Core");
    sa_file=fopen(registry,"w");if(!sa_file)return 1;
    fputs("sysadmin\tstn-labz\tSA\t1\t1\t1\n",sa_file);
    if(fclose(sa_file)!=0)return 1;
    check(digit_project_bind_security(root,"stn-labz","second","sysadmin",registry,mock,NULL),"successful Core binding");
    check(digit_project_security_channel_id(root,"stn-labz","second",id,sizeof(id)) && strcmp(id,"channel-123-1")==0,"Core channel ID persisted and resolved");
    check(!digit_project_bind_security(root,"stn-labz","second","sysadmin",registry,mock,NULL),"duplicate bound channel rejected");
    check(!digit_project_security_channel_id(root,"other-org","second",id,sizeof(id)),"organization scope required");
    check(!digit_project_security_channel_id(root,"stn-labz","second",id,1),"small output rejected");
    check(!digit_project_security_channel_id(root,"stn-labz","../second",id,sizeof(id)),"traversal rejected");
    {
        stnlabz_module_host_t host={0};
        int previous=calls;
        check(digit_project_provision(root,"stn-labz","third","poe","sysadmin",registry),
              "third project provisioned for trusted host");
        check(!digit_project_bind_security_host(root,"stn-labz","third","sysadmin",registry,NULL),
              "null host rejected");
        check(calls==previous,"null host never calls Core");
        check(!digit_project_bind_security_host(root,"stn-labz","third","sysadmin",registry,&host),
              "host without service callback denied");
        host.invoke_service=mock_host_service;
        check(!digit_project_bind_security_host(root,"stn-labz","third","poe",registry,&host),
              "regular admin rejected by trusted host adapter");
        check(calls==previous,"unauthorized host requests never reach Core");
        check(digit_project_bind_security_host(root,"stn-labz","third","sysadmin",registry,&host),
              "qualified SA binds security through trusted host");
        check(digit_project_security_channel_id(root,"stn-labz","third",id,sizeof(id)) &&
              strcmp(id,"channel-123-1")==0,"host Core response durably bound");
        check(!digit_project_bind_security_host(root,"stn-labz","third","sysadmin",registry,&host),
              "host adapter denies duplicate binding");
    }
    printf("\nProject Core bridge tests: %u executed, %u failed\n",count,failures);
    return failures?1:0;
}
