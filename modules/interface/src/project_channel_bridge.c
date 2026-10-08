#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"
#include "project_channel_bridge.h"

/* [AI:GPT-6 | 2026-10-08] Core-backed channel bridge.
 * A failed Core call or persistence step leaves no active binding.
 * Any Core channel orphan remains inaccessible without an independently
 * authorized ACL grant; no automatic permissions are issued.
 */
static int project_component(const char *name) {
    size_t i,n;
    if(!name)return 0;
    n=strnlen(name,DIGIT_PROJECT_ID_MAX);
    if(!n||n>=DIGIT_PROJECT_ID_MAX)return 0;
    for(i=0;i<n;++i)
        if(!((name[i]>='a'&&name[i]<='z')||(name[i]>='A'&&name[i]<='Z')||
             (name[i]>='0'&&name[i]<='9')||name[i]=='-'||name[i]=='_'))return 0;
    return 1;
}
static int project_open(const char *root,const char *org,const char *project) {
    int rootfd=-1,orgfd=-1,p=-1;
    struct stat st;
    if(!root||!project_component(org)||!project_component(project))return -1;
    rootfd=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(rootfd<0)goto end;
    orgfd=openat(rootfd,org,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(orgfd<0)goto end;
    p=openat(orgfd,project,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(p<0)goto end;
    if(fstat(p,&st)!=0 || !S_ISDIR(st.st_mode) || (st.st_mode&077)) {
        close(p);p=-1;
    }
end:
    if(orgfd>=0)close(orgfd);
    if(rootfd>=0)close(rootfd);
    return p;
}
int digit_project_security_channel_id(const char *root,const char *org,
    const char *project,char *channel_id,size_t capacity) {
    int dir=-1,fd=-1,ok=0;
    char text[DIGIT_CHANNEL_ID_MAX+2];
    ssize_t n;
    size_t i;
    struct stat st;
    if(channel_id&&capacity)channel_id[0]='\0';
    if(!channel_id || capacity<DIGIT_CHANNEL_ID_MAX ||
       !digit_project_security_ready(root,org,project))return 0;
    dir=project_open(root,org,project);
    if(dir<0)return 0;
    fd=openat(dir,"security_channel.id",O_RDONLY|O_NOFOLLOW);
    if(fd<0)goto end;
    if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || (st.st_mode&077)!=0)goto end;
    n=read(fd,text,sizeof(text));
    if(n<2 || n>=(ssize_t)sizeof(text) || text[n-1]!='\n')goto end;
    text[n-1]='\0';
    if(!project_component(text))goto end;
    for(i=0;i<(size_t)n-1;++i)
        if(text[i]=='\t'||text[i]=='\r')goto end;
    memcpy(channel_id,text,(size_t)n);
    ok=1;
end:
    if(fd>=0)close(fd);
    if(dir>=0)close(dir);
    return ok;
}
int digit_project_bind_security(const char *root,const char *org,
    const char *project,digit_project_core_invoke_t invoke,void *context) {
    digit_channel_create_request_t request;
    digit_channel_create_response_t result;
    char existing[DIGIT_CHANNEL_ID_MAX],entry[DIGIT_CHANNEL_ID_MAX+2];
    size_t used=0;
    int dir=-1,fd=-1,ok=0;
    int length;
    if(!invoke||!digit_project_security_ready(root,org,project))return 0;
    if(digit_project_security_channel_id(root,org,project,existing,sizeof(existing)))
        return 0;
    dir=project_open(root,org,project);
    if(dir<0)return 0;
    /* Reserve binding name before Core creation. Incomplete reservations
     * remain fail-closed and require operator reconciliation. */
    fd=openat(dir,"security_channel.id",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
    if(fd<0)goto end;
    memset(&request,0,sizeof(request));
    memset(&result,0,sizeof(result));
    length=snprintf(request.name,sizeof(request.name),"security-%s-%s",org,project);
    if(length<=0 || (size_t)length>=sizeof(request.name))goto end;
    if(!invoke(DIGIT_CHANNEL_SERVICE_CREATE,&request,sizeof(request),
               &result,sizeof(result),&used,context) ||
       used!=sizeof(result) || !result.created ||
       !project_component(result.channel.id))goto end;
    length=snprintf(entry,sizeof(entry),"%s\n",result.channel.id);
    if(length<=0 || (size_t)length>=sizeof(entry))goto end;
    if(write(fd,entry,(size_t)length)!=length || fsync(fd)!=0)goto end;
    if(fsync(dir)!=0)goto end;
    ok=1;
end:
    if(fd>=0)close(fd);
    /* Preserve incomplete reservation if Core was called: no repeat creates
     * potentially conflicting channels until an operator resolves it. */
    if(dir>=0)close(dir);
    return ok;
}
