#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"

/* [AI:GPT-6 | 2026-10-08] 1.3.5 ownership gate regression. */
static unsigned int total, failed;
static void check(int ok, const char *name)
{
    ++total;
    printf("%s 1.3.5 %02u - %s\n",ok?"PASS":"FAIL",total,name);
    if(!ok) ++failed;
}
int main(void)
{
    uid_t runtime=geteuid();
    uid_t other=runtime==1001?1002:1001;
    char root[]="/tmp/digit-owner-XXXXXX";
    char registry[256];
    FILE *f;
    check(digit_project_directory_owner_allowed(runtime,runtime),
          "runtime owner accepted");
    check(digit_project_directory_owner_allowed(0,runtime),
          "system owner accepted");
    check(!digit_project_directory_owner_allowed(other,runtime),
          "unrelated owner denied");
    check(!digit_project_directory_owner_allowed((uid_t)-1,runtime),
          "invalid owner denied");
    if(!mkdtemp(root)) return 1;
    snprintf(registry,sizeof(registry),"%s/sa.tsv",root);
    f=fopen(registry,"w");
    if(!f) return 1;
    fputs("sysadmin\tstn-labz\tSA\t1\t1\t1\n",f);
    if(fclose(f)!=0 || chmod(registry,0600)!=0) return 1;
    check(digit_project_provision(root,"stn-labz","digit","poe","sysadmin",registry),
          "trusted directory provisioning");
    check(digit_project_security_ready(root,"stn-labz","digit"),
          "trusted project readiness");
    check(digit_project_security_member(root,"stn-labz","digit","sysadmin"),
          "trusted membership");
    printf("Interface 1.3.5 milestone: %u executed, %u failed\n",total,failed);
    return failed!=0;
}
