#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "memory.h"
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif
#define MEMORY_DIR "/opt/digit/state/memory"
static const stnlabz_module_host_t *memory_host;
static int valid_actor(const char *actor){
 size_t i,n;
 if(!actor || !(n=strnlen(actor,DIGIT_MEMORY_ACTOR_MAX)) || n>=DIGIT_MEMORY_ACTOR_MAX)return 0;
 for(i=0;i<n;i++)if(!isalnum((unsigned char)actor[i])&&actor[i]!='_'&&actor[i]!='-')return 0;
 return 1;
}
static int memory_path(const char *dir,const char *actor,char *path,size_t cap){
 int n;
 if(!dir||!valid_actor(actor))return 0;
 n=snprintf(path,cap,"%s/%s.bin",dir,actor);
 return n>0&&(size_t)n<cap;
}
int digit_memory_put_at(const char *dir,const digit_memory_record_t *record){
 char path[512];int fd;ssize_t n;
 if(!record||!valid_actor(record->actor)||!memchr(record->text,0,sizeof(record->text))||
    !record->text[0]||!memory_path(dir,record->actor,path,sizeof(path)))return 0;
 fd=open(path,O_WRONLY|O_CREAT|O_APPEND|O_CLOEXEC|O_NOFOLLOW,0600);
 if(fd<0)return 0;
 if(flock(fd,LOCK_EX)!=0){close(fd);return 0;}
 n=write(fd,record->text,sizeof(record->text));
 if(n==(ssize_t)sizeof(record->text))n=fsync(fd)==0?n:-1;
 flock(fd,LOCK_UN);close(fd);
 return n==(ssize_t)sizeof(record->text);
}
int digit_memory_get_at(const char *dir,const char *actor,digit_memory_result_t *out){
 char path[512],item[DIGIT_MEMORY_TEXT_MAX];int fd;off_t size;ssize_t n;
 if(!out)return 0;
 memset(out,0,sizeof(*out));
 if(!memory_path(dir,actor,path,sizeof(path)))return 0;
 fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
 if(fd<0)return errno==ENOENT;
 if(flock(fd,LOCK_SH)!=0){close(fd);return 0;}
 size=lseek(fd,0,SEEK_END);
 if(size<0){flock(fd,LOCK_UN);close(fd);return 0;}
 /* Most recent record. No unverified partial record can be returned. */
 if(size>=(off_t)sizeof(item)){
  if(lseek(fd,(size/(off_t)sizeof(item)-1)*(off_t)sizeof(item),SEEK_SET)<0){
   flock(fd,LOCK_UN);close(fd);return 0;
  }
  n=read(fd,item,sizeof(item));
  if(n==(ssize_t)sizeof(item)&&memchr(item,0,sizeof(item))&&item[0]){
   memcpy(out->text,item,sizeof(item));out->ok=1;
  }
 }
 flock(fd,LOCK_UN);close(fd);
 return 1;
}
static stnlabz_module_result_t put_service(const void *req,size_t length,void *output,size_t cap,size_t *used,void *ctx){
 const digit_memory_record_t *r=req;digit_memory_result_t out={0};(void)ctx;
 if(!r||length!=sizeof(*r)||!output||!used||cap<sizeof(out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 out.ok=digit_memory_put_at(MEMORY_DIR,r);
 memcpy(output,&out,sizeof(out));*used=sizeof(out);return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t get_service(const void *req,size_t length,void *output,size_t cap,size_t *used,void *ctx){
 const digit_memory_record_t *r=req;digit_memory_result_t out={0};(void)ctx;
 if(!r||length!=sizeof(*r)||!output||!used||cap<sizeof(out)||!valid_actor(r->actor))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 if(!digit_memory_get_at(MEMORY_DIR,r->actor,&out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 memcpy(output,&out,sizeof(out));*used=sizeof(out);return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t qualify(stnlabz_module_qualification_result_t *q){
 if(!q)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 memset(q,0,sizeof(*q));q->tests_executed=3;
 q->tests_passed=(unsigned)valid_actor("poemei")+(unsigned)!valid_actor("../other")+
                 (unsigned)!valid_actor("");
 q->negative_test_executed=1;q->negative_test_passed=1;
 return q->tests_passed==3?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_QUALIFICATION;
}
static stnlabz_module_result_t start(const stnlabz_module_host_t *h){
 if(!h||!h->register_service||!h->unregister_service)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 if(!h->register_service(DIGIT_MEMORY_PUT_SERVICE,put_service,NULL))return STNLABZ_MODULE_ERR_START_FAILED;
 if(!h->register_service(DIGIT_MEMORY_GET_SERVICE,get_service,NULL)){
  h->unregister_service(DIGIT_MEMORY_PUT_SERVICE,NULL);return STNLABZ_MODULE_ERR_START_FAILED;
 }
 memory_host=h;return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t stop(void){
 int ok=1;
 if(memory_host){ok &= memory_host->unregister_service(DIGIT_MEMORY_GET_SERVICE,NULL);
                ok &= memory_host->unregister_service(DIGIT_MEMORY_PUT_SERVICE,NULL);}
 memory_host=NULL;return ok?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_STOP_FAILED;
}
static const stnlabz_module_descriptor_t descriptor={"memory","Digit Memory",1,0,0,
 STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,qualify,start,stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &descriptor;}
