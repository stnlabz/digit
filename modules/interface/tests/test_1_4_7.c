#include <stdio.h>
#include <string.h>
#include "message_results.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.7: exercise bounded,
 * fail-closed Core channel-message boundary decisions. */
static unsigned int count,failed;
static void check(int good,const char *name)
{
    ++count;
    printf("%s 1.4.7 %02u - %s\n",good?"PASS":"FAIL",count,name);
    if(!good)++failed;
}
int main(void)
{
    digit_channel_message_t messages[2]={{0}};
    strcpy(messages[0].id,"msg-1");
    strcpy(messages[0].channel_id,"channel-1");
    strcpy(messages[0].origin,"digit");
    strcpy(messages[0].body,"Evidence response");
    messages[1]=messages[0];
    strcpy(messages[1].id,"msg-2");
    check(digit_interface_messages_valid(messages,2,2,"channel-1"),
          "matching channel messages accepted");
    check(digit_interface_messages_valid(NULL,0,2,"channel-1"),
          "empty list accepted");
    check(!digit_interface_messages_valid(NULL,1,2,"channel-1"),
          "null nonempty list rejected");
    check(!digit_interface_messages_valid(messages,3,2,"channel-1"),
          "oversized service count rejected before access");
    check(!digit_interface_messages_valid(messages,2,2,"../channel"),
          "invalid requested channel denied");
    check(!digit_interface_messages_valid(messages,2,2,"other"),
          "cross-channel records denied");
    strcpy(messages[1].channel_id,"other");
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "mixed-channel reply denied");
    strcpy(messages[1].channel_id,"channel-1");
    strcpy(messages[1].id,"msg-1");
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "duplicate message identity denied");
    strcpy(messages[1].id,"msg-2");
    messages[1].id[0]=0;
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "missing message identity denied");
    strcpy(messages[1].id,"msg-2");
    strcpy(messages[1].id,"not/valid");
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "unsafe message identity denied");
    strcpy(messages[1].id,"msg-2");
    messages[1].origin[0]=0;
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "missing author rejected");
    strcpy(messages[1].origin,"digit");
    messages[1].body[0]=0;
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "empty message body rejected");
    strcpy(messages[1].body,"Second response");
    memset(messages[1].body,'A',sizeof(messages[1].body));
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "unterminated body rejected");
    strcpy(messages[1].body,"Second response");
    memset(messages[1].origin,'B',sizeof(messages[1].origin));
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "unterminated origin rejected");
    strcpy(messages[1].origin,"digit");
    memset(messages[1].channel_id,'C',sizeof(messages[1].channel_id));
    check(!digit_interface_messages_valid(messages,2,2,"channel-1"),
          "unterminated record channel rejected");
    strcpy(messages[1].channel_id,"channel-1");
    check(digit_interface_messages_valid(messages,2,2,"channel-1"),
          "valid records recover after malformed reply");
    check(!digit_interface_messages_valid(messages,1,0,"channel-1"),
          "zero-capacity result refuses entries");
    check(!digit_interface_messages_valid(messages,0,2,NULL),
          "missing request channel denied");
    printf("Interface 1.4.7 milestone: %u executed, %u failed\n",count,failed);
    return failed!=0;
}
