#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "memory.h"
#include "memory_response.h"
extern int digit_memory_put_at(const char *,const digit_memory_record_t *);
extern int digit_memory_get_at(const char *,const char *,digit_memory_result_t *);
int main(void){
 char dir[]="/tmp/digit-memory-test-XXXXXX",file[512];
 digit_memory_record_t record={0};
 digit_memory_result_t result={0};
 assert(mkdtemp(dir)!=NULL);
 snprintf(record.actor,sizeof(record.actor),"test_account");
 snprintf(record.text,sizeof(record.text),"I maintain a project.");
 assert(digit_memory_put_at(dir,&record));
 assert(digit_memory_get_at(dir,"test_account",&result));
 assert(result.ok&&strcmp(result.text,record.text)==0);
 assert(digit_memory_get_at(dir,"another_account",&result)&&!result.ok);
 snprintf(record.text,sizeof(record.text),"I also created another application.");
 assert(digit_memory_put_at(dir,&record));
 assert(digit_memory_get_at(dir,"test_account",&result));
 assert(result.ok&&strcmp(result.text,record.text)==0);
 strcpy(record.actor,"../escape");
 assert(!digit_memory_put_at(dir,&record));
 snprintf(file,sizeof(file),"%s/test_account.bin",dir);unlink(file);rmdir(dir);
 {
  char natural[DIGIT_MEMORY_TEXT_MAX]={0};
  const char introduction[]="I am Ava, I am an engineer and I manage the laboratory.";
  assert(digit_memory_natural_reply(introduction,natural,sizeof(natural)));
  assert(strcmp(natural,"Hi Ava! You told me that you are an engineer and you manage the laboratory.")==0);
  assert(digit_memory_natural_reply("my project is complete",natural,sizeof(natural)));
  assert(strcmp(natural,"You told me that your project is complete.")==0);
 }
 puts("Memory storage and natural-response tests passed");
 return 0;
}
