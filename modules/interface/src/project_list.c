#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_list.h"
#include "project_provision.h"
/* [AI:GPT-6 | 2026-10-09] Only private directories containing validated
 * READY project records appear; never traverse or disclose other orgs. */
static int valid_id(const char *s){
    size_t i,n;
    if(!s)return 0;
    n=strnlen(s,DIGIT_PROJECT_ID_MAX);
    if(n==0||n>=DIGIT_PROJECT_ID_MAX)return 0;
    for(i=0;i<n;i++){
        char c=s[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
    }
    return 1;
}
static int private_dir(int fd){
    struct stat s;
    return fd>=0 && fstat(fd,&s)==0 && S_ISDIR(s.st_mode) &&
       (s.st_mode&077)==0 && digit_project_directory_owner_allowed(s.st_uid,geteuid());
}
int digit_project_list_scoped(const char *root,const char *organization,
                             char *output,size_t capacity){
    int base=-1,org=-1,ok=0,opened=0;
    DIR *dir=NULL;
    struct dirent *entry;
    size_t count=0,off=0;
    if(output&&capacity)output[0]=0;
    if(!root||!valid_id(organization)||!output||capacity<32)return 0;
    base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(base))goto done;
    org=openat(base,organization,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(org<0 && errno==ENOENT){
        int n=snprintf(output,capacity,"{\"organization\":\"%s\",\"projects\":[]}\n",organization);
        ok=n>0&&(size_t)n<capacity;goto done;
    }
    if(!private_dir(org))goto done;
    dir=fdopendir(org);
    if(!dir)goto done;
    opened=1;org=-1;
    off=(size_t)snprintf(output,capacity,"{\"organization\":\"%s\",\"projects\":[",organization);
    if(off>=capacity)goto done;
    errno=0;
    while((entry=readdir(dir))!=NULL){
        struct stat st;
        int n;
        if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0)continue;
        if(!valid_id(entry->d_name) ||
           fstatat(dirfd(dir),entry->d_name,&st,AT_SYMLINK_NOFOLLOW)!=0||
           !S_ISDIR(st.st_mode)||(st.st_mode&077)!=0||
           !digit_project_directory_owner_allowed(st.st_uid,geteuid()))
            goto done;
        if(!digit_project_security_ready(root,organization,entry->d_name))goto done;
        n=snprintf(output+off,capacity-off,"%s\"%s\"",count?",":"",entry->d_name);
        if(n<0||(size_t)n>=capacity-off)goto done;
        off+=(size_t)n;count++;
    }
    if(errno!=0)goto done;
    if(snprintf(output+off,capacity-off,"]}\n")<0||
       strlen(output)>=capacity-1)goto done;
    ok=1;
done:
    if(dir && closedir(dir)!=0)ok=0;
    if(!opened && org>=0)close(org);
    if(base>=0)close(base);
    if(!ok && output&&capacity)output[0]=0;
    return ok;
}
