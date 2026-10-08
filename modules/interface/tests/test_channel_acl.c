#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "channel_acl.h"

/* [AI:GPT-6 | 2026-10-08] Operator ACL fixture regression tests. */
static unsigned int count=0,failed=0;
static void check(int pass,const char *name) {
    ++count; if(pass)printf("PASS %u %s\n",count,name);
    else {++failed;printf("FAIL %u %s\n",count,name);}
}
static int write_acl(const char *path,const char *entry) {
    FILE *f=fopen(path,"w");int ok;
    if(!f)return 0;
    ok=fputs(entry,f)>=0;
    return fclose(f)==0 && ok;
}
int main(void) {
    char path[]="/tmp/digit-acl-XXXXXX";
    int fd=mkstemp(path);
    if(fd<0)return 1;
    close(fd);chmod(path,0600);
    check(write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\n"),"fixture installed");
    check(digit_channel_acl_check_file(path,"ezra","general"),"eligible member");
    check(!digit_channel_acl_check_file(path,"poe","general"),"other user denied");
    check(!digit_channel_acl_check_file(path,"ezra","security"),"other channel denied");
    check(!digit_channel_acl_check_file(NULL,"ezra","general"),"null path denied");
    check(!digit_channel_acl_check_file(path,NULL,"general"),"null identity denied");
    check(!digit_channel_acl_check_file(path,"ezra",NULL),"null channel denied");
    chmod(path,0644);
    check(!digit_channel_acl_check_file(path,"ezra","general"),"unsafe permission denied");
    chmod(path,0600);
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t0\t1\t1\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"qualification required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t0\t1\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"assignment required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t0\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"mission qualification required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t0\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"grant required");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\nezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"duplicate grants denied");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1\nbad\trow\n");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"malformed registry denied");
    write_acl(path,"ezra\tstn-labz\tdigit\tgeneral\t1\t1\t1\t1");
    check(!digit_channel_acl_check_file(path,"ezra","general"),"unterminated line denied");
    unlink(path);
    check(!digit_channel_acl_check_file(path,"ezra","general"),"missing registry denied");
    printf("Channel ACL: %u executed, %u failed\n",count,failed);
    return failed?1:0;
}
