#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "channel_acl.h"
#include "project_provision.h"
#include "project_channel_bridge.h"

/* [AI:GPT-6 | 2026-10-08] Interface 1.3.6: scoped Core channel
 * ownership and authorization negative validation. */
static unsigned int total,failed;
static void check(int ok,const char *label) {
    ++total;
    printf("%s 1.3.6 %02u - %s\n",ok?"PASS":"FAIL",total,label);
    if(!ok)++failed;
}
static int write_private(const char *path,const char *data) {
    FILE *f=fopen(path,"w");
    int good;
    if(!f)return 0;
    good=fputs(data,f)>=0 && fflush(f)==0;
    if(fclose(f)!=0)good=0;
    return good && chmod(path,0600)==0;
}
static int provision_fixture(const char *root,const char *org,
                             const char *project,const char *id) {
    char dir[320],p[384],file[448],row[256];
    snprintf(dir,sizeof(dir),"%s/%s",root,org);
    if(mkdir(dir,0700)!=0 && access(dir,F_OK)!=0)return 0;
    snprintf(p,sizeof(p),"%s/%s",dir,project);
    if(mkdir(p,0700)!=0)return 0;
    snprintf(file,sizeof(file),"%s/project.tsv",p);
    snprintf(row,sizeof(row),"%s\t%s\tpoe\n",org,project);
    if(!write_private(file,row))return 0;
    snprintf(file,sizeof(file),"%s/security.tsv",p);
    if(!write_private(file,"security\trestricted\tdigit\nsecurity\trestricted\tsysadmin\n"))return 0;
    snprintf(file,sizeof(file),"%s/READY",p);
    if(!write_private(file,"security-initialized\n"))return 0;
    snprintf(file,sizeof(file),"%s/security_channel.id",p);
    snprintf(row,sizeof(row),"%s\n",id);
    return write_private(file,row);
}
int main(void) {
    char root[]="/tmp/digit-scope-136-XXXXXX";
    char acl[]="/tmp/digit-scope-acl-XXXXXX";
    char sa[]="/tmp/digit-scope-sa-XXXXXX";
    char owner_org[DIGIT_PROJECT_ID_MAX],owner_project[DIGIT_PROJECT_ID_MAX];
    int fd;
    if(!mkdtemp(root))return 1;
    fd=mkstemp(acl);if(fd<0)return 1;close(fd);
    fd=mkstemp(sa);if(fd<0)return 1;close(fd);
    if(!write_private(sa,"sysadmin\torg-a\tSA\t1\t1\t1\n"
                         "sysadmin\torg-b\tSA\t1\t1\t1\n"))return 1;
    check(provision_fixture(root,"org-a","alpha","secure-a"),
          "owner A project fixture");
    check(provision_fixture(root,"org-a","beta","secure-b"),
          "same-organization project fixture");
    check(provision_fixture(root,"org-b","alpha","secure-c"),
          "other-organization project fixture");
    check(digit_project_security_owner(root,"secure-a",owner_org,sizeof(owner_org),
          owner_project,sizeof(owner_project))==1 &&
          strcmp(owner_org,"org-a")==0 && strcmp(owner_project,"alpha")==0,
          "Core channel resolves to one project owner");
    check(write_private(acl,"sysadmin\torg-a\talpha\tsecure-a\t1\t1\t1\t1\n"),
          "legitimate ACL grant written");
    check(digit_channel_acl_check_scoped(acl,root,sa,"sysadmin","secure-a"),
          "owner A authorized");
    check(write_private(acl,"sysadmin\torg-a\tbeta\tsecure-a\t1\t1\t1\t1\n"),
          "same-organization borrowing fixture");
    check(!digit_channel_acl_check_scoped(acl,root,sa,"sysadmin","secure-a"),
          "cross-project Core channel borrowing denied");
    check(write_private(acl,"sysadmin\torg-b\talpha\tsecure-a\t1\t1\t1\t1\n"),
          "cross-organization borrowing fixture");
    check(!digit_channel_acl_check_scoped(acl,root,sa,"sysadmin","secure-a"),
          "cross-organization Core channel borrowing denied");
    check(write_private(acl,"sysadmin\torg-a\talpha\tsecure-a\t1\t1\t1\t1\n"),
          "owner A grant restored");
    check(digit_channel_acl_check_scoped(acl,root,sa,"sysadmin","secure-a"),
          "legitimate scope restored");
    check(!digit_channel_acl_check_scoped(acl,root,sa,"sysadmin","secure-b"),
          "other project ID cannot use owner A ACL row");
    check(write_private(acl,"sysadmin\torg-b\talpha\tsecure-c\t1\t1\t1\t1\n"),
          "owner B grant written");
    check(digit_channel_acl_check_scoped(acl,root,sa,"sysadmin","secure-c"),
          "separate organization owner remains authorized");
    printf("Interface 1.3.6 milestone: %u executed, %u failed\n",total,failed);
    return failed!=0;
}
