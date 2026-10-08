#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"

/* [AI:GPT-6 | 2026-10-08] READY is published only after both records.
 * Incomplete folders are NOT provisioned projects.
 */
static int project_name(const char *name) {
    size_t i,n;
    if(!name)return 0;
    n=strnlen(name,DIGIT_PROJECT_ID_MAX);
    if(!n||n>=DIGIT_PROJECT_ID_MAX)return 0;
    for(i=0;i<n;i++)
        if(!((name[i]>='a'&&name[i]<='z')||
             (name[i]>='A'&&name[i]<='Z')||
             (name[i]>='0'&&name[i]<='9')||
             name[i]=='-'||name[i]=='_'))return 0;
    return 1;
}
static int private_dir(int fd) {
    struct stat s;
    return fd>=0 && fstat(fd,&s)==0 && S_ISDIR(s.st_mode) &&
           (s.st_mode&077)==0;
}
static int open_private(int dir,const char *name) {
    int fd=openat(dir,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(fd)){if(fd>=0)close(fd);return -1;}
    return fd;
}
static int record(int dir,const char *name,const char *value) {
    int fd=openat(dir,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
    size_t n=strlen(value);
    int good=0;
    if(fd<0)return 0;
    if(write(fd,value,n)==(ssize_t)n && fsync(fd)==0)good=1;
    if(close(fd)!=0)good=0;
    return good;
}
int digit_project_security_ready(const char *root,const char *org,const char *project) {
    int base=-1,o=-1,p=-1,good=0;
    struct stat marker,security;
    if(!root||!project_name(org)||!project_name(project))return 0;
    base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(base))goto end;
    o=open_private(base,org);if(o<0)goto end;
    p=open_private(o,project);if(p<0)goto end;
    good=fstatat(p,"READY",&marker,AT_SYMLINK_NOFOLLOW)==0 &&
         S_ISREG(marker.st_mode) &&
         fstatat(p,"security.tsv",&security,AT_SYMLINK_NOFOLLOW)==0 &&
         S_ISREG(security.st_mode);
end:
    if(p>=0)close(p);
    if(o>=0)close(o);
    if(base>=0)close(base);
    return good;
}
int digit_project_provision(const char *root,const char *org,
                             const char *project,const char *admin) {
    int base=-1,o=-1,p=-1,good=0;
    char metadata[256],security[256];
    if(!root||!project_name(org)||!project_name(project)||
       !project_name(admin)||strcmp(admin,"digit")==0)return 0;
    base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(base))goto end;
    if(mkdirat(base,org,0700)!=0 && errno!=EEXIST)goto end;
    o=open_private(base,org);if(o<0)goto end;
    if(mkdirat(o,project,0700)!=0)goto end;
    p=open_private(o,project);if(p<0)goto end;
    if(snprintf(metadata,sizeof(metadata),"%s\t%s\t%s\n",org,project,admin)<0)goto end;
    if(snprintf(security,sizeof(security),"security\trestricted\tdigit\nsecurity\trestricted\t%s\n",admin)<0)goto end;
    if(!record(p,"project.tsv",metadata)||
       !record(p,"security.tsv",security)||
       !record(p,"READY","security-initialized\n"))goto end;
    good=fsync(p)==0;
end:
    if(p>=0)close(p);
    if(o>=0)close(o);
    if(base>=0)close(base);
    return good;
}
