#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include "project_provision.h"
#include "project_channel_bridge.h"
#include "security_sa.h"

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
/* [AI:GPT-6 | 2026-10-08] Interface 1.3.6: channel binding
 * must enforce the same directory-owner boundary as provisioning. */
static int trusted_project_dir(int fd) {
    struct stat st;
    return fd>=0 && fstat(fd,&st)==0 && S_ISDIR(st.st_mode) &&
           (st.st_mode&077)==0 &&
           digit_project_directory_owner_allowed(st.st_uid,geteuid());
}
static int project_open(const char *root,const char *org,const char *project) {
    int rootfd=-1,orgfd=-1,p=-1;
    if(!root||!project_component(org)||!project_component(project))return -1;
    rootfd=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!trusted_project_dir(rootfd))goto end;
    orgfd=openat(rootfd,org,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!trusted_project_dir(orgfd))goto end;
    p=openat(orgfd,project,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!trusted_project_dir(p)) {if(p>=0)close(p);p=-1;}
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
    /* [AI:GPT-6 | 2026-10-08] A binding must be privately owned by
     * root or the runtime and must not have another hard-link alias. */
    if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) ||
       (st.st_mode&077)!=0 || st.st_nlink!=1 ||
       (st.st_uid!=0 && st.st_uid!=geteuid()))goto end;
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
/* [AI:GPT-6 | 2026-10-08] Resolve a protected Core ID to its unique
 * project owner. Corrupt or unreadable inventory fails closed. A missing
 * root means no project registry has been provisioned yet.
 */
static int ordinary_binding_matches(int project_fd,const char *channel);
static int security_owner_scan(const char *root,const char *channel_id,
    char *organization,size_t org_capacity,char *project,size_t project_capacity,
    const char *reserved_org,const char *reserved_project)
{
    DIR *organizations=NULL,*projects=NULL;
    struct dirent *o,*p;
    int rootfd=-1,orgfd=-1,found=0;
    char candidate[DIGIT_CHANNEL_ID_MAX];
    if(organization&&org_capacity)organization[0]=0;
    if(project&&project_capacity)project[0]=0;
    if(!root||!project_component(channel_id)||!organization||!project||
       !org_capacity||!project_capacity)return -1;
    rootfd=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(rootfd<0)return errno==ENOENT?0:-1;
    {
        struct stat st;
        if(fstat(rootfd,&st)!=0||!S_ISDIR(st.st_mode)||(st.st_mode&077)!=0||
           !digit_project_directory_owner_allowed(st.st_uid,geteuid()))
            goto failure;
    }
    organizations=fdopendir(rootfd);
    if(!organizations)goto failure;
    rootfd=-1;
    for(;;){
        errno=0;
        o=readdir(organizations);
        if(!o){if(errno!=0)goto failure;break;}
        struct stat st;
        if(o->d_name[0]=='.' && (!o->d_name[1] ||
           (o->d_name[1]=='.'&&!o->d_name[2])))continue;
        if(!project_component(o->d_name))goto failure;
        if(fstatat(dirfd(organizations),o->d_name,&st,AT_SYMLINK_NOFOLLOW)!=0 ||
           !S_ISDIR(st.st_mode)||(st.st_mode&077)!=0||
           !digit_project_directory_owner_allowed(st.st_uid,geteuid()))goto failure;
        orgfd=openat(dirfd(organizations),o->d_name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
        if(orgfd<0)goto failure;
        projects=fdopendir(orgfd);
        if(!projects)goto failure;
        orgfd=-1;
        for(;;){
            errno=0;
            p=readdir(projects);
            if(!p){if(errno!=0)goto failure;break;}
            if(p->d_name[0]=='.' && (!p->d_name[1] ||
               (p->d_name[1]=='.'&&!p->d_name[2])))continue;
            if(!project_component(p->d_name))goto failure;
            if(fstatat(dirfd(projects),p->d_name,&st,AT_SYMLINK_NOFOLLOW)!=0||
               !S_ISDIR(st.st_mode)||(st.st_mode&077)!=0||
           !digit_project_directory_owner_allowed(st.st_uid,geteuid()))goto failure;
            /* [AI:GPT-6 | 2026-10-08] Incomplete or corrupted project
             * records cannot be ignored during ownership discovery.
             * An omitted owner could otherwise permit access through
             * a different ACL scope. Fail closed for the whole scan. */
            if(!digit_project_security_ready(root,o->d_name,p->d_name))
                goto failure;
            /* An ordinary channel is owned by exactly one project too. */
            {
                int pd=openat(dirfd(projects),p->d_name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
                int ordinary;
                if(pd<0)goto failure;
                ordinary=ordinary_binding_matches(pd,channel_id);close(pd);
                if(ordinary<0)goto failure;
                if(ordinary==1){
                    if(++found>1||strlen(o->d_name)>=org_capacity||
                       strlen(p->d_name)>=project_capacity)goto failure;
                    strcpy(organization,o->d_name);strcpy(project,p->d_name);
                }
            }
            if(digit_project_security_channel_id(root,o->d_name,p->d_name,
                    candidate,sizeof(candidate))){
                if(strcmp(candidate,channel_id)==0){
                    if(++found>1||strlen(o->d_name)>=org_capacity||
                       strlen(p->d_name)>=project_capacity)goto failure;
                    strcpy(organization,o->d_name);
                    strcpy(project,p->d_name);
                }
            }else{
                /* An unbound project is valid; an existing damaged binding
                 * is not. Do not permit shadowing of protected IDs. */
                if(fstatat(dirfd(projects),p->d_name,&st,AT_SYMLINK_NOFOLLOW)!=0)
                    goto failure;
                {
                    int pd=openat(dirfd(projects),p->d_name,
                                  O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
                    if(pd<0)goto failure;
                    if(fstatat(pd,"security_channel.id",&st,AT_SYMLINK_NOFOLLOW)==0){
                        /* Only this invocation's new empty reservation is
                         * exempt. All ordinary ACL scans remain fail-closed. */
                        int own_reservation=reserved_org&&reserved_project&&
                            strcmp(o->d_name,reserved_org)==0&&
                            strcmp(p->d_name,reserved_project)==0&&
                            S_ISREG(st.st_mode)&&st.st_size==0&&
                            (st.st_mode&077)==0;
                        close(pd);
                        if(!own_reservation)goto failure;
                        continue;
                    }
                    if(errno!=ENOENT){close(pd);goto failure;}
                    close(pd);
                }
            }
        }
        closedir(projects);projects=NULL;
    }
    closedir(organizations);
    return found;
failure:
    if(projects)closedir(projects);
    if(orgfd>=0)close(orgfd);
    if(organizations)closedir(organizations);
    if(rootfd>=0)close(rootfd);
    organization[0]=0;project[0]=0;
    return -1;
}

/* [AI:GPT-6 | 2026-10-09] Validate a private ordinary channel binding. */
static int ordinary_binding_read(int project_fd,const char *filename,char *out,size_t cap){
 char value[DIGIT_CHANNEL_ID_MAX+2];struct stat st;int fd;ssize_t n;
 if(!project_component(filename)||!out||cap<DIGIT_CHANNEL_ID_MAX)return 0;
 fd=openat(project_fd,filename,O_RDONLY|O_NOFOLLOW);
 if(fd<0)return 0;
 if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_nlink!=1||
    (st.st_mode&077)||(st.st_uid!=0&&st.st_uid!=geteuid())||
    st.st_size<2||st.st_size>(off_t)(sizeof(value)-1)){close(fd);return 0;}
 n=read(fd,value,sizeof(value));
 close(fd);
 if(n!=st.st_size||value[n-1]!='\n')return 0;
 value[n-1]=0;
 if(!project_component(value)||strlen(value)>=cap)return 0;
 strcpy(out,value);return 1;
}
static int ordinary_binding_matches(int project_fd,const char *channel){
 DIR *entries;struct dirent *entry;int duplicate_fd,found=0;
 char bound[DIGIT_CHANNEL_ID_MAX];
 duplicate_fd=dup(project_fd);if(duplicate_fd<0)return -1;
 entries=fdopendir(duplicate_fd);if(!entries){close(duplicate_fd);return -1;}
 for(;;){
  size_t n;errno=0;entry=readdir(entries);
  if(!entry){if(errno)found=-1;break;}
  n=strlen(entry->d_name);
  if(n<12||strncmp(entry->d_name,"channel-",8)!=0||
     strcmp(entry->d_name+n-3,".id")!=0)continue;
  if(!ordinary_binding_read(project_fd,entry->d_name,bound,sizeof(bound))){
   found=-1;break;
  }
  if(!strcmp(bound,channel))found++;
  if(found>1){found=-1;break;}
 }
 closedir(entries);return found;
}
int digit_project_channel_match(const char *root,const char *org,
 const char *project,const char *channel){
 int fd,match;
 if(!project_component(channel)||!digit_project_security_ready(root,org,project))return 0;
 fd=project_open(root,org,project);if(fd<0)return 0;
 match=ordinary_binding_matches(fd,channel);close(fd);
 return match==1;
}
int digit_project_security_owner(const char *root,const char *channel_id,
    char *organization,size_t org_capacity,char *project,size_t project_capacity)
{
    return security_owner_scan(root,channel_id,organization,org_capacity,
                               project,project_capacity,NULL,NULL);
}

int digit_project_bind_security(const char *root,const char *org,
    const char *project,const char *actor,const char *sa_registry,
    digit_project_core_invoke_t invoke,void *context) {
    digit_channel_create_request_t request;
    digit_channel_create_response_t result;
    char existing[DIGIT_CHANNEL_ID_MAX],entry[DIGIT_CHANNEL_ID_MAX+2];
    char owner_org[DIGIT_PROJECT_ID_MAX],owner_project[DIGIT_PROJECT_ID_MAX];
    size_t used=0;
    int dir=-1,fd=-1,lockfd=-1,ok=0;
    int length;
    struct stat root_stat;
    /* [AI:GPT-6 | 2026-10-08] No service call or reservation is allowed
     * without independently verified current SA + project membership. */
    if(!invoke||!digit_project_security_ready(root,org,project)||
       !digit_security_sa_verify(sa_registry,org,actor)||
       !digit_project_security_member(root,org,project,actor))return 0;
    /* [AI:GPT-6 | 2026-10-08] Serialize all project bindings using the
     * protected project root itself: no extra inventory files or lock
     * cleanup, including across independent Interface processes.
     * Non-blocking failure denies creation without invoking Core.
     */
    lockfd=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(lockfd<0)return 0;
    if(fstat(lockfd,&root_stat)!=0 || !S_ISDIR(root_stat.st_mode) ||
       (root_stat.st_mode&077)!=0 ||
       !digit_project_directory_owner_allowed(root_stat.st_uid,geteuid()) ||
       flock(lockfd,LOCK_EX|LOCK_NB)!=0)
        goto end;
    if(digit_project_security_channel_id(root,org,project,existing,sizeof(existing)))
        goto end;
    /* Refuse new creation while protected inventory is inconsistent. */
    if(digit_project_security_owner(root,"channel-probe",owner_org,sizeof(owner_org),
                                   owner_project,sizeof(owner_project))<0)goto end;
    dir=project_open(root,org,project);
    if(dir<0)goto end;
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
    /* [AI:GPT-6 | 2026-10-08] A Core-issued channel ID is not proof
     * of unique ownership. Never publish a duplicate protected binding. */
    if(security_owner_scan(root,result.channel.id,
           owner_org,sizeof(owner_org),owner_project,sizeof(owner_project),
           org,project)!=0)
        goto end;
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
    if(lockfd>=0)close(lockfd);
    return ok;
}

/* [AI:GPT-6 | 2026-10-08] Qualified internal module-host entry point.
 * Callback context is only a trusted module host; all actor authorization
 * stays in digit_project_bind_security. Never exposed as an HTTP route.
 */
static int project_host_invoke(const char *service,const void *request,
    size_t request_size,void *response,size_t response_size,
    size_t *response_used,void *context)
{
    const stnlabz_module_host_t *host=context;
    if(!host || !host->invoke_service)return 0;
    return host->invoke_service(service,request,request_size,
                               response,response_size,response_used)==STNLABZ_MODULE_OK;
}
int digit_project_bind_security_host(const char *root,const char *organization,
    const char *project,const char *actor,const char *sa_registry,
    const stnlabz_module_host_t *host)
{
    if(!host||!host->invoke_service)return 0;
    return digit_project_bind_security(root,organization,project,actor,
                                       sa_registry,project_host_invoke,(void *)host);
}

/* [AI:GPT-6 | 2026-10-09] An ordinary channel is provisioned only
 * inside an existing qualified project. A Core orphan remains inaccessible
 * if any protected persistence step fails. */
int digit_project_channel_create_host(const char *root,const char *org,
 const char *project,const char *name,const char *actor,const char *sa_registry,
 const stnlabz_module_host_t *host,char *channel,size_t capacity){
 char filename[64],owner_org[DIGIT_PROJECT_ID_MAX],owner_project[DIGIT_PROJECT_ID_MAX];
 char entry[DIGIT_CHANNEL_ID_MAX+2],existing[DIGIT_CHANNEL_ID_MAX];
 digit_channel_create_request_t request;digit_channel_create_response_t result;
 int lock=-1,dir=-1,fd=-1,ok=0,n;size_t used=0;
 struct stat st;
 if(!channel||capacity<DIGIT_CHANNEL_ID_MAX||!host||!host->invoke_service||
    !project_component(name)||strlen(name)>40||
    !digit_project_security_ready(root,org,project)||
    !digit_security_sa_verify(sa_registry,org,actor)||
    !digit_project_security_member(root,org,project,actor))return 0;
 channel[0]=0;
 n=snprintf(filename,sizeof(filename),"channel-%s.id",name);
 if(n<1||(size_t)n>=sizeof(filename))return 0;
 lock=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(lock<0||fstat(lock,&st)!=0||!S_ISDIR(st.st_mode)||
    (st.st_mode&077)||!digit_project_directory_owner_allowed(st.st_uid,geteuid())||
    flock(lock,LOCK_EX|LOCK_NB)!=0)goto done;
 if(digit_project_security_owner(root,"channel-probe",owner_org,sizeof(owner_org),
                                 owner_project,sizeof(owner_project))<0)goto done;
 dir=project_open(root,org,project);if(dir<0)goto done;
 if(fstatat(dir,filename,&st,AT_SYMLINK_NOFOLLOW)==0||errno!=ENOENT)goto done;
 fd=openat(dir,filename,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(fd<0)goto done;
 memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));
 n=snprintf(request.name,sizeof(request.name),"channel-%s-%s-%s",org,project,name);
 if(n<1||(size_t)n>=sizeof(request.name))goto done;
 if(host->invoke_service(DIGIT_CHANNEL_SERVICE_CREATE,&request,sizeof(request),
       &result,sizeof(result),&used)!=STNLABZ_MODULE_OK||
    used!=sizeof(result)||!result.created||
    !project_component(result.channel.id))goto done;
 if(digit_project_security_owner(root,result.channel.id,owner_org,sizeof(owner_org),
      owner_project,sizeof(owner_project))!=0)goto done;
 n=snprintf(entry,sizeof(entry),"%s\n",result.channel.id);
 if(n<2||(size_t)n>=sizeof(entry)||write(fd,entry,(size_t)n)!=n||
    fsync(fd)!=0||fsync(dir)!=0)goto done;
 strcpy(channel,result.channel.id);ok=1;
done:
 if(fd>=0)close(fd);
 if(dir>=0)close(dir);
 if(lock>=0)close(lock);
 return ok;
}
