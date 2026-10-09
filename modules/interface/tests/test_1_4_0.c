#include <stdio.h>
#include <string.h>
#include "controlled_communication.h"

/* [AI:GPT-6 | 2026-10-08] 1.4.0 controlled, authenticated
 * channel submission and Core-persistence acknowledgement checks. */
static unsigned int count,failed;
static void check(int good,const char *label)
{
    ++count;
    printf("%s 1.4.0 %02u - %s\n",good?"PASS":"FAIL",count,label);
    if(!good)++failed;
}
int main(void)
{
    digit_channel_message_t message={0};
    char receipt[256],tiny[8],too_long[DIGIT_CHANNEL_MESSAGE_MAX+2];
    strcpy(message.id,"message-101");
    strcpy(message.channel_id,"channel-101");
    strcpy(message.origin,"operator");
    strcpy(message.body,"report");
    check(digit_controlled_message_valid("channel-101","operator","report"),
          "authorized-origin message format accepted");
    check(digit_controlled_message_valid("channel-101","digit","response"),
          "Digit response origin format accepted");
    check(!digit_controlled_message_valid(NULL,"operator","report"),
          "missing channel identifier denied");
    check(!digit_controlled_message_valid("channel-101",NULL,"report"),
          "missing actor identity denied");
    check(!digit_controlled_message_valid("channel-101","operator",NULL),
          "missing message body denied");
    check(!digit_controlled_message_valid("channel-101","operator",""),
          "empty body denied");
    check(!digit_controlled_message_valid("channel/other","operator","report"),
          "invalid channel scope denied");
    check(!digit_controlled_message_valid("channel-101","bad/actor","report"),
          "untrusted origin denied");
    memset(too_long,'x',sizeof(too_long));
    too_long[sizeof(too_long)-1]=0;
    check(!digit_controlled_message_valid("channel-101","operator",too_long),
          "oversized message body denied");
    check(digit_controlled_receipt(&message,"channel-101","operator",
                                   receipt,sizeof(receipt)),
          "matching Core persistence receipt generated");
    check(strcmp(receipt,
          "{\"message_id\":\"message-101\",\"channel_id\":\"channel-101\",\"status\":\"persisted\"}")==0,
          "receipt identifies persisted message without delivery claim");
    check(!digit_controlled_receipt(&message,"channel-102","operator",
                                    receipt,sizeof(receipt)),
          "cross-channel acknowledgement rejected");
    check(!digit_controlled_receipt(&message,"channel-101","other",
                                    receipt,sizeof(receipt)),
          "mismatched author acknowledgement rejected");
    check(!digit_controlled_receipt(&message,"channel-101","operator",
                                    tiny,sizeof(tiny)),
          "truncated acknowledgement rejected");
    check(!digit_controlled_receipt(NULL,"channel-101","operator",
                                    receipt,sizeof(receipt)),
          "missing Core acknowledgement rejected");
    strcpy(message.id,"bad\"id");
    check(!digit_controlled_receipt(&message,"channel-101","operator",
                                    receipt,sizeof(receipt)),
          "unsafe Core message ID rejected");
    strcpy(message.id,"message-101");
    strcpy(message.origin,"digit");
    check(!digit_controlled_receipt(&message,"channel-101","operator",
                                    receipt,sizeof(receipt)),
          "unattributed Core response rejected");
    printf("Interface 1.4.0 milestone: %u executed, %u failed\n",count,failed);
    return failed!=0;
}
