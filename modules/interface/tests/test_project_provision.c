#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"

/* [AI:GPT-6 | 2026-10-08] Project bootstrap negative tests. */
static unsigned int total=0,failures=0;
static void check(int ok,const char *name) {
    ++total;
    if(ok)printf("PASS %02u - %s\n",total,name);
    else {++failures;printf("FAIL %02u - %s\n",total,name);}
}
int main(void) {
    char root[]="/tmp/digit-project-XXXXXX";
    char project[256],security[300],ready[300],org[256];
    FILE *f;
    char line[120];
    if(!mkdtemp(root))return 1;
    snprintf(project,sizeof(project),"%s/stn-labz/digit",root);
    snprintf(org,sizeof(org),"%s/stn-labz",root);
    snprintf(security,sizeof(security),"%s/security.tsv",project);
    snprintf(ready,sizeof(ready),"%s/READY",project);
    check(!digit_project_security_ready(root,"stn-labz","digit"),"absent project not ready");
    check(!digit_project_provision(root,"stn-labz","digit","digit"),"Digit cannot self-approve founding administrator");
    check(!digit_project_provision(root,"bad/name","digit","poe"),"invalid organization denied");
    check(!digit_project_provision(root,"stn-labz","../bad","poe"),"path traversal denied");
    check(!digit_project_provision(root,"stn-labz","digit","bad/user"),"invalid founder denied");
    check(digit_project_provision(root,"stn-labz","digit","poe"),"project provision succeeds");
    check(digit_project_security_ready(root,"stn-labz","digit"),"restricted security record published");
    check(!digit_project_provision(root,"stn-labz","digit","poe"),"duplicate project denied");
    check(!digit_project_security_ready(root,"stn-labz","other"),"unregistered project denied");
    f=fopen(security,"r");
    check(f!=NULL,"security membership file exists");
    if(f) {
        int digit=0,founder=0;
        while(fgets(line,sizeof(line),f)) {
            if(strcmp(line,"security\trestricted\tdigit\n")==0)digit=1;
            if(strcmp(line,"security\trestricted\tpoe\n")==0)founder=1;
        }
        fclose(f);
        check(digit,"Digit initially assigned to restricted security");
        check(founder,"founding administrator initially assigned");
    }
    check(!digit_project_security_ready(root,"stn-labz","bad/name"),"unsafe channel path denied");
    check(!digit_project_security_ready(root,"elsewhere","digit"),"organization isolation");
    check(!digit_project_provision(NULL,"stn-labz","other","poe"),"null root denied");
    printf("\nProject provisioning tests: %u executed, %u failed\n",total,failures);
    return failures!=0;
}
