#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "channel_acl.h"

/* [AI:GPT-6 | 2026-10-08] Operator ACL fixture regression tests. */
static unsigned int count=0,failed=0;
static void check(int pass,const char *name) {
    ++count; if(pass)printf("PASS %u %s\n",count,name);
    else {++failed;printf("FAIL %u %s\n",count,name);}
}
static int write_acl(const char *path,const char *entry) {
    FILE *f=fopen(path,"w");int ok;
    if(!f)return 0;
    ok=fputs(entry,f)>=0;
    return fclose(f)==0 && ok;
}
int main(void) {
    char path[]="/tmp/digit-acl-XXXXXX";
    char root[]="/tmp/digit-acl-project-XXXXXX";
    char org[256],project[320],file[384],sa[320];
    int fd=mkstemp(path);
    if(fd<0)return 1;
    close(fd);chmod(path,0600);
    check(write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\n"),"fixture installed");
    check(digit_channel_acl_check_file(path,"ezra","general"),"eligible member");
    check(!digit_channel_acl_check_file(path,"poe","general"),"other user denied");
    check(!digit_channel_acl_check_file(path,"ezra","security"),"other channel denied");
    check(!digit_channel_acl_check_file(NULL,"ezra","general"),"null path denied");
    check(!digit_channel_acl_check_file(path,NULL,"general"),"null identity denied");
    check(!digit_channel_acl_check_file(path,"ezra",NULL),"null channel denied");
    chmod(path,0644);
    check(!digit_channel_acl_check_file(path,"ezra","general"),"unsafe permission denied");
    chmod(path,0600);
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t0\t1\t1\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"qualification required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t0\t1\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"assignment required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t0\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"mission qualification required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t0\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"grant required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\nezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"duplicate grants denied");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\nbad\trow\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"malformed registry denied");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"unterminated line denied");
    /* [AI:GPT-6 | 2026-10-08] Security channels are not ordinary grants. */
    if(!mkdtemp(root))return 1;
    snprintf(org,sizeof(org),"%s/stn-labz",root);
    snprintf(project,sizeof(project),"%s/digit",org);
    snprintf(sa,sizeof(sa),"%s/sa.tsv",root);
    mkdir(org,0700);mkdir(project,0700);
    snprintf(file,sizeof(file),"%s/READY",project);
    write_acl(file,"security-initialized\n");
    if(chmod(file,0600)!=0)return 1;
    snprintf(file,sizeof(file),"%s/project.tsv",project);
    write_acl(file,"stn-labz\tdigit\tregular\n");
    if(chmod(file,0600)!=0)return 1;
    snprintf(file,sizeof(file),"%s/security.tsv",project);
    write_acl(file,"security\trestricted\tdigit\n");
    if(chmod(file,0600)!=0)return 1;
    write_acl(sa,"regular\tstn-labz\tADMIN\t1\t1\t1\n"
                 "sysadmin\tstn-labz\tSA\t1\t1\t1\n");
    /* [AI:GPT-6 | 2026-10-08] Match protected 0600 files created by
     * production provisioning, independent of the operator umask. */
    if(chmod(sa,0600)!=0)return 1;
    write_acl(path,"regular\tstn-labz\tdigit\tchannel-123-1\t1\t1\t1\t1\n");
    check(!digit_channel_acl_check_scoped(path,root,sa,"regular","channel-123-1"),"no access before Core security binding");
    snprintf(file,sizeof(file),"%s/security_channel.id",project);
    write_acl(file,"channel-123-1\n");
    if(chmod(file,0600)!=0)return 1;
    check(!digit_channel_acl_check_scoped(path,root,sa,"regular","channel-123-1"),"regular admin denied even with explicit security grant");
    write_acl(path,"sysadmin\tstn-labz\tdigit\tchannel-123-1\t1\t1\t1\t1\n");
    check(!digit_channel_acl_check_scoped(path,root,sa,"sysadmin","channel-123-1"),"SA without security membership denied");
    snprintf(file,sizeof(file),"%s/security.tsv",project);
    write_acl(file,"security\trestricted\tdigit\nsecurity\trestricted\tsysadmin\n");
    if(chmod(file,0600)!=0)return 1;

    check(digit_channel_acl_check_scoped(path,root,sa,"sysadmin","channel-123-1"),"verified SA with grant accesses bound security channel");
    write_acl(file,"security\trestricted\tdigit\n");
    check(!digit_channel_acl_check_scoped(path,root,sa,"sysadmin","channel-123-1"),"removed project membership denies active SA");
    write_acl(file,"security\trestricted\tdigit\nsecurity\trestricted\tsysadmin\n");
    check(digit_channel_acl_check_scoped(path,root,sa,"sysadmin","channel-123-1"),"restored project membership grants active SA");
    write_acl(file,"security\trestricted\tsysadmin\nsecurity\trestricted\tsysadmin\n");
    check(!digit_channel_acl_check_scoped(path,root,sa,"sysadmin","channel-123-1"),"duplicate project security membership denied");
    write_acl(file,"security\trestricted\tsysadmin\nmalformed\n");
    check(!digit_channel_acl_check_scoped(path,root,sa,"sysadmin","channel-123-1"),"malformed project security roster denied");
    write_acl(file,"security\trestricted\tdigit\nsecurity\trestricted\tsysadmin\n");
    write_acl(sa,"sysadmin\tstn-labz\tSA\t1\t0\t1\n");
    check(!digit_channel_acl_check_scoped(path,root,sa,"sysadmin","channel-123-1"),"SA assignment revoked immediately denies access");
    unlink(path);
    check(!digit_channel_acl_check_file(path,"ezra","general"),"missing registry denied");
    printf("Channel ACL: %u executed, %u failed\n",count,failed);
    return failed?1:0;
}
