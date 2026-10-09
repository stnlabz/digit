#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "grant_store.h"
#include "channel_acl.h"
#include "project_provision.h"
#include "project_channel_bridge.h"
#include "security_sa.h"
/* [AI:GPT-6 | 2026-10-09] Initial 1.6.0 grant writer is intentionally
 * restricted to a bound Security channel. Other channels require their
 * own explicit validated project ownership before grants can be issued.
 * No actor-supplied flags, SA bypass, or permission escalation.
 * Serialize using the protected directory inode; write a unique private
 * temporary file and atomically rename after full validation.
 */
int digit_grant_security_scoped(const char *path,const char *root,const char *sa_registry,
 const char *actor,const digit_grant_request_t *req){
 char binding[64],record[512],tmp[128],line[1024];
 char parent[512],*slash;
 int dir=-1,fd=-1,source=-1,target=-1,ok=0,existing=0,created=0;
 FILE *stream=NULL;
 struct stat st;
 size_t n;unsigned long attempts;
 if(!path||!root||!sa_registry||!actor||!req)return 0;
 if(!digit_security_sa_verify(sa_registry,req->organization,actor)||
    !digit_project_security_ready(root,req->organization,req->project)||
    !digit_project_security_member(root,req->organization,req->project,actor)||
    !digit_security_sa_verify(sa_registry,req->organization,req->user)||
    !digit_project_security_member(root,req->organization,req->project,req->user)||
    !digit_project_security_channel_id(root,req->organization,req->project,binding,sizeof(binding))||
    strcmp(binding,req->channel)!=0)return 0;
 if(strlen(path)>=sizeof(parent))return 0;
 strcpy(parent,path);slash=strrchr(parent,'/');
 if(!slash||slash==parent)return 0;
 *slash++=0;
 if(strcmp(slash,"channel_grants.tsv")!=0)return 0;
 dir=open(parent,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(dir<0||fstat(dir,&st)!=0||!S_ISDIR(st.st_mode)||(st.st_mode&077)!=0||
   !digit_project_directory_owner_allowed(st.st_uid,geteuid())||
   flock(dir,LOCK_EX|LOCK_NB)!=0)goto finish;
 source=openat(dir,slash,O_RDONLY|O_NOFOLLOW);
 if(source>=0){
  if(fstat(source,&st)!=0||!S_ISREG(st.st_mode)||st.st_nlink!=1||
    (st.st_mode&077)!=0||(st.st_uid!=0&&st.st_uid!=geteuid())||
    st.st_size<=0||st.st_size>8*1024*1024)goto finish;
  stream=fdopen(source,"r");if(!stream)goto finish;
  source=-1;existing=1;
 }else if(errno!=ENOENT)goto finish;
 n=(size_t)snprintf(record,sizeof(record),"%s\t%s\t%s\t%s\t1\t1\t1\t1\n",
       req->user,req->organization,req->project,req->channel);
 if(n>=sizeof(record))goto finish;
 for(attempts=0;attempts<16;attempts++){
  if(snprintf(tmp,sizeof(tmp),".channel-grants-%ld-%lu.tmp",(long)getpid(),attempts)>=(int)sizeof(tmp))goto finish;
  target=openat(dir,tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
  if(target>=0){created=1;break;}
  if(errno!=EEXIST)goto finish;
 }
 if(target<0)goto finish;
 if(existing){
  while(fgets(line,sizeof(line),stream)){
   char copy[1024],*field[8],*p;size_t i,len=strlen(line);
   if(!len||line[len-1]!='\n')goto finish;
   strcpy(copy,line);copy[len-1]=0;p=copy;
   for(i=0;i<8;i++){
    char *tab;
    field[i]=p;if(i==7)break;
    tab=strchr(p,'\t');if(!tab)goto finish;
    *tab=0;p=tab+1;
   }
   if(strchr(field[7],'\t'))goto finish;
   for(i=4;i<8;i++)if(strcmp(field[i],"0")&&strcmp(field[i],"1"))goto finish;
   for(i=0;i<4;i++){
    size_t k;len=strlen(field[i]);
    if(!len||len>=128)goto finish;
    for(k=0;k<len;k++){unsigned char c=(unsigned char)field[i][k];
      if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))goto finish;
    }
   }
   if(strcmp(field[0],req->user)==0&&strcmp(field[3],req->channel)==0)goto finish;
   len=strlen(line);
   if(write(target,line,len)!=(ssize_t)len)goto finish;
  }
  if(ferror(stream))goto finish;
 }
 if(write(target,record,n)!=(ssize_t)n||fsync(target)!=0)goto finish;
 if(close(target)!=0){target=-1;goto finish;}target=-1;
 if(renameat(dir,tmp,dir,slash)!=0||fsync(dir)!=0)goto finish;
 created=0;ok=1;
finish:
 if(stream)fclose(stream);
 if(source>=0)close(source);
 if(target>=0)close(target);
 if(created&&dir>=0)unlinkat(dir,tmp,0);
 if(dir>=0)close(dir);
 return ok;
}
