#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sa_management.h"
#include "security_sa.h"
/* [AI:GPT-6 | 2026-10-09] Fail-closed, same-organization SA management.
 * A role change requires an existing target record with qualification and
 * mission-qualification already established. The actor must currently be SA.
 * Serialized complete-file replacement preserves all unrelated records. */
static int ident(const char *s){
 size_t i,n;
 if(!s)return 0;n=strnlen(s,64);
 if(!n||n>=64)return 0;
 for(i=0;i<n;++i){char c=s[i];if(!((c>='a'&&c<='z')||
 (c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))return 0;}
 return 1;
}
static int parse(char *line,char **fields){
 size_t i,n=strlen(line);
 if(!n||line[n-1]!='\n')return 0;
 line[n-1]=0;
 for(i=0;i<5;i++){char *p;
  fields[i]=line;
  p=strchr(line,'\t');if(!p)return 0;*p=0;line=p+1;
 }
 fields[5]=line;
 if(strchr(line,'\t'))return 0;
 if(!ident(fields[0])||!ident(fields[1])||
   (strcmp(fields[2],"SA")&&strcmp(fields[2],"ADMIN")))return 0;
 for(i=3;i<6;i++)if(strcmp(fields[i],"0")&&strcmp(fields[i],"1"))return 0;
 return 1;
}
static int read_safe(int fd,char **buf,size_t *length){
 struct stat st;size_t off=0;ssize_t n;char *p;
 if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_nlink!=1||
    st.st_size<=0||st.st_size>8*1024*1024||(st.st_mode&077)||
    (st.st_uid!=0&&st.st_uid!=geteuid()))return 0;
 p=calloc((size_t)st.st_size+1,1);if(!p)return 0;
 while(off<(size_t)st.st_size){
  n=read(fd,p+off,(size_t)st.st_size-off);
  if(n<=0){free(p);return 0;}off+=(size_t)n;
 }
 *buf=p;*length=off;return 1;
}
int digit_sa_list(const char *registry,const char *org,const char *actor,
 char *json,size_t cap){
 int fd=-1,ok=0;char *buf=NULL,*copy=NULL,*line,*save=NULL;size_t len,off=0,count=0;
 if(!registry||!ident(org)||!ident(actor)||!json||cap<48||
    !digit_security_sa_verify(registry,org,actor))return 0;
 fd=open(registry,O_RDONLY|O_NOFOLLOW);if(fd<0)goto done;
 if(!read_safe(fd,&buf,&len))goto done;
 copy=strdup(buf);if(!copy)goto done;
 off=(size_t)snprintf(json,cap,"{\"organization\":\"%s\",\"assignments\":[",org);
 if(off>=cap)goto done;
 for(line=strtok_r(copy,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){
  char record[512],*fields[6];int n;
  if(strlen(line)+2>sizeof(record))goto done;
  snprintf(record,sizeof(record),"%s\n",line);
  if(!parse(record,fields))goto done;
  if(strcmp(fields[1],org))continue;
  n=snprintf(json+off,cap-off,"%s{\"user\":\"%s\",\"role\":\"%s\",\"qualified\":%s,\"assigned\":%s,\"mission_qualified\":%s}",
   count?",":"",fields[0],fields[2],
   !strcmp(fields[3],"1")?"true":"false",
   !strcmp(fields[4],"1")?"true":"false",
   !strcmp(fields[5],"1")?"true":"false");
  if(n<0||(size_t)n>=cap-off)goto done;off+=(size_t)n;++count;
 }
 if(off+4>cap)goto done;
 strcpy(json+off,"]}\n");ok=1;
done:
 free(copy);free(buf);if(fd>=0)close(fd);
 return ok;
}
int digit_sa_change(const char *registry,const char *org,const char *actor,
 const char *user,int assign){
 char parent[512],*slash,*buf=NULL,*out=NULL,*line,*save=NULL;
 char temp[96],*fields[6];size_t len,used=0,found=0,active=0,target_sa=0;
 int bootstrap=assign==2;
 int dir=-1,fd=-1,tmp=-1,ok=0;struct stat st;unsigned long seq=0;
 if(!registry||!ident(org)||!ident(actor)||!ident(user)||
    (assign!=0&&assign!=1&&assign!=2)||strlen(registry)>=sizeof(parent))return 0;
 snprintf(parent,sizeof(parent),"%s",registry);
 slash=strrchr(parent,'/');if(!slash||slash==parent)return 0;
 *slash++=0;
 dir=open(parent,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(dir<0||fstat(dir,&st)!=0||!S_ISDIR(st.st_mode)||
    (st.st_mode&077)||(st.st_uid!=0&&st.st_uid!=geteuid())||
    flock(dir,LOCK_EX)!=0)goto done;
 if(bootstrap){
  /* [AI:GPT-6 | 2026-10-09] Explicit one-time founder bootstrap, not a general privilege bridge. */
  if(strcmp(org,"team-chaos")||strcmp(actor,"poemei")||strcmp(user,"poemei")||
     !digit_security_sa_verify(registry,"stn-labz",actor))goto done;
 }else if(!digit_security_sa_verify(registry,org,actor))goto done;
 fd=openat(dir,slash,O_RDONLY|O_NOFOLLOW);
 if(fd<0||!read_safe(fd,&buf,&len))goto done;
 out=calloc(len+512,1);if(!out)goto done;
 for(line=strtok_r(buf,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){
  char record[512];int n;size_t i;
  if(strlen(line)+2>sizeof(record))goto done;
  snprintf(record,sizeof(record),"%s\n",line);
  if(!parse(record,fields))goto done;
  if(!bootstrap&&!strcmp(fields[0],actor)&&!strcmp(fields[1],org)&&
      !strcmp(fields[2],"SA")&&!strcmp(fields[3],"1")&&
      !strcmp(fields[4],"1")&&!strcmp(fields[5],"1"))active++;
  if(!strcmp(fields[1],org)&&!strcmp(fields[2],"SA")&&
     !strcmp(fields[3],"1")&&!strcmp(fields[4],"1")&&!strcmp(fields[5],"1"))target_sa++;
  if(!strcmp(fields[0],user)&&!strcmp(fields[1],org)){
   found++;
   /* Existing qualification is necessary; assigning cannot mint it. */
   if(assign&&(!strcmp(fields[3],"0")||!strcmp(fields[5],"0")))goto done;
   /* Prevent an SA from accidentally removing their own final authority. */
   if(!assign&&!strcmp(user,actor))goto done;
   fields[2]=assign?"SA":"ADMIN";fields[4]=assign?"1":"0";
  }
  n=snprintf(out+used,len+512-used,"%s\t%s\t%s\t%s\t%s\t%s\n",
   fields[0],fields[1],fields[2],fields[3],fields[4],fields[5]);
  if(n<0||(size_t)n>=len+512-used)goto done;
  used+=(size_t)n;
  for(i=0;i<6;i++)if(!fields[i][0])goto done;
 }
 if((bootstrap ? target_sa!=0 : active!=1)||found!=1)goto done;
 for(seq=0;seq<16;seq++){
  snprintf(temp,sizeof(temp),".security_sa.%ld.%lu.tmp",(long)getpid(),seq);
  tmp=openat(dir,temp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
  if(tmp>=0)break;if(errno!=EEXIST)goto done;
 }
 if(tmp<0)goto done;
 if(write(tmp,out,used)!=(ssize_t)used||fsync(tmp)!=0)goto done;
 if(renameat(dir,temp,dir,slash)!=0||fsync(dir)!=0)goto done;
 ok=1;
done:
 if(tmp>=0){close(tmp);if(!ok)unlinkat(dir,temp,0);}
 if(fd>=0)close(fd);if(dir>=0)close(dir);
 free(buf);free(out);return ok;
}
