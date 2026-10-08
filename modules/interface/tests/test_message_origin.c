#include <stdio.h>
#include <string.h>
#include "message_origin.h"

/* [AI:GPT-6 | 2026-10-08] Authenticated origin validation tests. */
static int tests=0,failed=0;
static void check(int ok,const char *label) {
    ++tests;
    if(ok)printf("PASS %02d - %s\n",tests,label);
    else {++failed;printf("FAIL %02d - %s\n",tests,label);}
}
int main(void) {
    char author[DIGIT_CHANNEL_ORIGIN_MAX];
    char long_id[DIGIT_CHANNEL_ORIGIN_MAX+2];
    memset(long_id,'a',sizeof(long_id));
    long_id[sizeof(long_id)-1]=0;
    check(digit_message_origin_from_identity("poe",author,sizeof(author)),"valid employee identity accepted");
    check(strcmp(author,"poe")==0,"attribution matches authenticated identity");
    check(digit_message_origin_from_identity("sysadmin_01",author,sizeof(author)),"SA username accepted");
    check(strcmp(author,"sysadmin_01")==0,"SA attribution preserved");
    check(!digit_message_origin_from_identity(NULL,author,sizeof(author)),"missing identity denied");
    check(!digit_message_origin_from_identity("",author,sizeof(author)),"empty identity denied");
    check(!digit_message_origin_from_identity("poe",NULL,sizeof(author)),"missing output denied");
    check(!digit_message_origin_from_identity("poe",author,1),"undersized destination denied");
    check(!digit_message_origin_from_identity(long_id,author,sizeof(author)),"oversized identity denied without truncation");
    check(!digit_message_origin_from_identity("bad/user",author,sizeof(author)),"invalid identity characters rejected");
    check(!digit_message_origin_from_identity("admin\tclaimed",author,sizeof(author)),"delimiter injection rejected");
    check(!digit_message_origin_from_identity("admin\nclaimed",author,sizeof(author)),"line injection rejected");
    printf("Message origin: %d executed, %d failed\n",tests,failed);
    return failed?1:0;
}
