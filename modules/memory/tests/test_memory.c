#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "memory.h"
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
 strcpy(record.actor,"../escape");
 assert(!digit_memory_put_at(dir,&record));
 snprintf(file,sizeof(file),"%s/test_account.bin",dir);unlink(file);rmdir(dir);
 puts("Memory storage tests passed");
 return 0;
}
