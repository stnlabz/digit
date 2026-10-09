#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "alerts_channel.h"
#include "project_provision.h"
#include "security_sa.h"
#include "core_services.h"
/* [AI:GPT-6 | 2026-10-09] Exact protected Alerts binding file, with
 * project-local reservation before invoking Core. An interrupted creation
 * remains reserved, preventing duplicate Core channel creation. */
static int valid(const char *s){
 size_t i,n;
 if(!s)return 0;n=strnlen(s,64);
 if(!n||n>=64)return 0;
 for(i=0;i<n;++i){char c=s[i];
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
   (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
 }return 1;
}
static int private_dir(int fd){
 struct stat st;
 return fd>=0&&fstat(fd,&st)==0&&S_ISDIR(st.st_mode)&&
  !(st.st_mode&077)&&digit_project_directory_owner_allowed(st.st_uid,geteuid());
}
static int open_project(const char *root,const char *org,const char *project){
 int base=-1,o=-1,p=-1;
 if(!root||!valid(org)||!valid(project))return -1;
 base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(!private_dir(base))goto done;
 o=openat(base,org,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(!private_dir(o))goto done;
 p=openat(o,project,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(!private_dir(p)){if(p>=0)close(p);p=-1;}
done:
 if(o>=0)close(o);if(base>=0)close(base);
 return p;
}
static int lookup_fd(int dir,char *out,size_t cap){
 char text[DIGIT_CHANNEL_ID_MAX+2];
 struct stat st;
 ssize_t n;
 int fd,ok=0;
 if(!out||cap<DIGIT_CHANNEL_ID_MAX)return 0;
 out[0]=0;
 fd=openat(dir,"alerts_channel.id",O_RDONLY|O_NOFOLLOW);
 if(fd<0)return 0;
 if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_nlink!=1||
    (st.st_mode&077)||(st.st_uid!=0&&st.st_uid!=geteuid()))goto done;
 n=read(fd,text,sizeof(text));
 if(n<2||n>=(ssize_t)sizeof(text)||text[n-1]!='\n'||st.st_size!=n)goto done;
 text[n-1]=0;
 if(!valid(text))goto done;
 memcpy(out,text,(size_t)n);
 ok=1;
done:
 close(fd);return ok;
}
int digit_alerts_channel_lookup(const char *root,const char *org,const char *project,
 char *channel,size_t cap){
 int p,ok;
 if(!channel||cap<DIGIT_CHANNEL_ID_MAX||
   !digit_project_security_ready(root,org,project))return 0;
 p=open_project(root,org,project);
 if(p<0)return 0;
 ok=lookup_fd(p,channel,cap);
 close(p);return ok;
}
int digit_alerts_channel_ensure(const char *root,const char *org,const char *project,
 const char *actor,const char *sa_registry,const stnlabz_module_host_t *host,
 char *channel,size_t cap){
 int p=-1,lock=-1,fd=-1,ok=0,n;
 digit_channel_create_request_t request;
 digit_channel_create_response_t result;
 size_t used=0;
 char text[DIGIT_CHANNEL_ID_MAX+2];
 if(!channel||cap<DIGIT_CHANNEL_ID_MAX||!host||!host->invoke_service||
    !digit_security_sa_verify(sa_registry,org,actor)||
    !digit_project_security_member(root,org,project,actor))return 0;
 p=open_project(root,org,project);if(p<0)return 0;
 lock=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(!private_dir(lock)||flock(lock,LOCK_EX|LOCK_NB)!=0)goto done;
 if(lookup_fd(p,channel,cap)){ok=1;goto done;}
 /* An existing malformed or incomplete reservation cannot be overwritten. */
 {
  struct stat st;
  if(fstatat(p,"alerts_channel.id",&st,AT_SYMLINK_NOFOLLOW)==0||errno!=ENOENT)goto done;
 }
 fd=openat(p,"alerts_channel.id",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(fd<0)goto done;
 memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));
 n=snprintf(request.name,sizeof(request.name),"alerts-%s-%s",org,project);
 if(n<=0||(size_t)n>=sizeof(request.name))goto done;
 if(host->invoke_service(DIGIT_CHANNEL_SERVICE_CREATE,&request,sizeof(request),
    &result,sizeof(result),&used)!=STNLABZ_MODULE_OK||
    used!=sizeof(result)||!result.created||!valid(result.channel.id))goto done;
 n=snprintf(text,sizeof(text),"%s\n",result.channel.id);
 if(n<=0||(size_t)n>=sizeof(text))goto done;
 if(write(fd,text,(size_t)n)!=n||fsync(fd)!=0||fsync(p)!=0)goto done;
 memcpy(channel,result.channel.id,strlen(result.channel.id)+1);
 ok=1;
done:
 if(fd>=0)close(fd);
 if(lock>=0)close(lock);
 if(p>=0)close(p);
 return ok;
}
