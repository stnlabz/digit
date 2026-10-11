#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ai_platform_auth.h"
static int tests,failed;
static void check(int ok,const char *name){
 tests++;if(!ok){failed++;fprintf(stderr,"FAIL %s\n",name);}
}
static int write_data(const char *path,const char *data){
 FILE *f=fopen(path,"w");int ok;
 if(!f)return 0;ok=fputs(data,f)>=0&&fclose(f)==0;
 return ok;
}
int main(void){
 char path[]="/tmp/digit-ai-test-XXXXXX";int fd=mkstemp(path);
 digit_ai_platform_permissions_t p={0};
 if(fd<0)return 1;close(fd);chmod(path,0600);
 check(write_data(path,"ai.chatgpt\t1\t1\t1\t0\t0\t1\n"),"fixture");
 check(digit_ai_platform_permissions(path,"ai.chatgpt",&p)&&p.dm&&p.diagnostics&&p.developer&&!p.learning&&!p.memory_write,"explicit permissions");
 check(!digit_ai_platform_permissions(path,"ai.other",&p),"unknown identity denied");
 check(write_data(path,"ai.chatgpt\t0\t1\t1\t0\t0\t1\n")&&
       !digit_ai_platform_permissions(path,"ai.chatgpt",&p),"inactive denied");
 check(write_data(path,"ai.chatgpt\t1\t1\t1\t0\t0\t1\nai.chatgpt\t1\t1\t1\t0\t0\t1\n")&&
       !digit_ai_platform_permissions(path,"ai.chatgpt",&p),"duplicate denied");
 check(write_data(path,"ai.chatgpt\t1\t1\t2\t0\t0\t1\n")&&
       !digit_ai_platform_permissions(path,"ai.chatgpt",&p),"invalid flag denied");
 check(write_data(path,"ai.chatgpt\t1\t1\t1\t0\t0\t1\n")&&
       chmod(path,0644)==0&&
       !digit_ai_platform_permissions(path,"ai.chatgpt",&p),"world-readable denied");
 chmod(path,0600);
 {char linkpath[96];int n=snprintf(linkpath,sizeof(linkpath),"%s-link",path);
  if(n>0&&(size_t)n<sizeof(linkpath)&&symlink(path,linkpath)==0){
   check(!digit_ai_platform_permissions(linkpath,"ai.chatgpt",&p),"registry symlink denied");
   unlink(linkpath);
  }else check(0,"symlink fixture");
 }
 unlink(path);
 check(!digit_ai_platform_permissions(path,"ai.chatgpt",&p),"missing registry denied");
 printf("AI platform permissions: %d checks, %d failures\n",tests,failed);
 return failed!=0;
}
