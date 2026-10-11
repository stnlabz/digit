#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <crypt.h>
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
 /* Independent high-entropy API-key path: active AI accounts only. */
 {
  char registry[]="/tmp/digit-ai-reg-XXXXXX";
  char credentials[]="/tmp/digit-ai-cred-XXXXXX";
  const char *key="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  char actor[64]={0},row[1024],*hash;
  int rf=mkstemp(registry),cf=mkstemp(credentials);
  check(rf>=0&&cf>=0,"API key fixtures opened");
  if(rf>=0&&cf>=0){
   close(rf);close(cf);chmod(registry,0600);chmod(credentials,0600);
   hash=crypt(key,"$6$ai-platform-test$");
   check(hash!=NULL,"credential crypt hash produced");
   if(hash){
    snprintf(row,sizeof(row),"ai.chatgpt\t1\t%s\n",hash);
    check(write_data(credentials,row),"credential fixture written");
    check(write_data(registry,"ai.chatgpt\t1\t1\t1\t0\t0\t0\n"),"AI policy written");
    check(digit_ai_platform_verify_key(registry,credentials,key,actor)&&
          strcmp(actor,"ai.chatgpt")==0,"API bearer authenticates unique AI identity");
    check(!digit_ai_platform_verify_key(registry,credentials,
          "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",actor),
          "incorrect bearer denied");
    check(write_data(registry,"ai.chatgpt\t0\t1\t1\t0\t0\t0\n")&&
          !digit_ai_platform_verify_key(registry,credentials,key,actor),
          "revoked AI policy denies bearer");
    check(write_data(registry,"human\t1\t1\t1\t0\t0\t0\n")&&
          !digit_ai_platform_verify_key(registry,credentials,key,actor),
          "non-AI principal rejected");
   }
   unlink(registry);unlink(credentials);
  }else{if(rf>=0){close(rf);unlink(registry);}if(cf>=0){close(cf);unlink(credentials);}}
 }
 printf("AI platform permissions: %d checks, %d failures\n",tests,failed);
 return failed!=0;
}
