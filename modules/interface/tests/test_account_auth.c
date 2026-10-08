#define _GNU_SOURCE
#include <crypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "account_auth.h"

/* [AI:GPT-6 | 2026-10-08] Test credentials exist only in temporary test file. */
static int checks=0,failures=0;
static void check(int pass,const char *name) {
    ++checks;
    if(pass)printf("PASS %02d - %s\n",checks,name);
    else {++failures;printf("FAIL %02d - %s\n",checks,name);}
}
static int write_accounts(const char *path,const char *text) {
    FILE *f=fopen(path,"w");if(!f)return 0;
    if(fputs(text,f)<0){fclose(f);return 0;}
    return fclose(f)==0;
}
int main(void) {
    char path[]="/tmp/digit-account-test-XXXXXX",record[1024];
    const char *hash;
    int fd=mkstemp(path);
    if(fd<0)return 1;
    close(fd);
    chmod(path,0600);
    hash=crypt("correct-horse","$6$digit-test-salt$");
    if(!hash || hash[0]=='*'){unlink(path);return 1;}
    snprintf(record,sizeof(record),"ezra\t1\t%s\n",hash);
    check(write_accounts(path,record),"protected account file provisioned");
    check(digit_account_verify_file(path,"ezra","correct-horse"),"correct password authenticates active identity");
    check(!digit_account_verify_file(path,"ezra","incorrect"),"wrong password denied");
    check(!digit_account_verify_file(path,"other","correct-horse"),"unknown user denied");
    check(digit_account_active_file(path,"ezra"),"active identity confirmed");
    check(!digit_account_active_file(path,"other"),"unknown active record denied");
    check(!digit_account_verify_file(path,"ezra",""),"empty password denied");
    check(!digit_account_verify_file(path,"ezra",NULL),"null password denied");
    check(!digit_account_verify_file(path,"", "correct-horse"),"empty identity denied");
    check(!digit_account_verify_file(NULL,"ezra","correct-horse"),"null file denied");
    chmod(path,0644);
    check(!digit_account_verify_file(path,"ezra","correct-horse"),"world-readable file denied");
    chmod(path,0600);
    snprintf(record,sizeof(record),"ezra\t0\t%s\n",hash);
    write_accounts(path,record);
    check(!digit_account_verify_file(path,"ezra","correct-horse"),"disabled account denied");
    check(!digit_account_active_file(path,"ezra"),"disabled account no longer active");
    snprintf(record,sizeof(record),"ezra\t1\t%s\nezra\t1\t%s\n",hash,hash);
    write_accounts(path,record);
    check(!digit_account_verify_file(path,"ezra","correct-horse"),"duplicate records denied");
    write_accounts(path,"ezra\t1\tmalformed\nother\tjunk\n");
    check(!digit_account_verify_file(path,"ezra","correct-horse"),"malformed account store denied");
    write_accounts(path,"ezra\t1\tmalformed");
    check(!digit_account_active_file(path,"ezra"),"unterminated record denied");
    unlink(path);
    check(!digit_account_verify_file(path,"ezra","correct-horse"),"missing file denied");
    printf("\nAccount authentication tests: %d executed, %d failed\n",checks,failures);
    return failures?1:0;
}
