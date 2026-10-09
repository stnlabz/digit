#include <stdio.h>
#include <string.h>
#include "channel_results.h"
/* [AI:GPT-6 | 2026-10-08] Interface 1.4.10:
 * channel list and exact lookup service-boundary regression checks. */
static unsigned int executed,failed;
static void check(int ok,const char *why)
{
    ++executed;
    printf("%s 1.4.10 %02u - %s\n",ok?"PASS":"FAIL",executed,why);
    if(!ok)++failed;
}
static digit_channel_t example(void)
{
    digit_channel_t c={0};
    strcpy(c.id,"channel-1");
    strcpy(c.name,"Engineering");
    c.active=1;
    return c;
}
int main(void)
{
    digit_channel_t channels[2]={{0}};
    channels[0]=example();
    channels[1]=example();
    strcpy(channels[1].id,"channel-2");
    check(digit_interface_channels_valid(channels,2,2),
          "valid channel list accepted");
    check(digit_interface_channels_valid(NULL,0,2),
          "empty list accepted");
    check(!digit_interface_channels_valid(NULL,1,2),
          "missing nonempty list rejected");
    check(!digit_interface_channels_valid(channels,3,2),
          "oversized list count rejected before access");
    check(!digit_interface_channels_valid(channels,1,0),
          "nonempty list with zero capacity rejected");
    check(digit_interface_channel_exact_valid(&channels[0],1,"channel-1"),
          "matching exact channel accepted");
    check(digit_interface_channel_exact_valid(NULL,0,"channel-1"),
          "not-found channel accepted without payload");
    check(!digit_interface_channel_exact_valid(&channels[0],2,"channel-1"),
          "nonboolean found flag rejected");
    check(!digit_interface_channel_exact_valid(NULL,1,"channel-1"),
          "missing found channel rejected");
    check(!digit_interface_channel_exact_valid(&channels[0],1,"channel-2"),
          "mismatched exact channel identity rejected");
    check(!digit_interface_channel_exact_valid(NULL,0,"../invalid"),
          "unsafe requested channel denied");
    check(!digit_interface_channel_exact_valid(NULL,0,NULL),
          "null requested channel denied");
    strcpy(channels[1].id,"channel-1");
    check(!digit_interface_channels_valid(channels,2,2),
          "duplicate channel identity rejected");
    strcpy(channels[1].id,"bad/path");
    check(!digit_interface_channels_valid(channels,2,2),
          "unsafe channel identity rejected");
    strcpy(channels[1].id,"channel-2");
    channels[1].name[0]=0;
    check(!digit_interface_channels_valid(channels,2,2),
          "empty channel name rejected");
    strcpy(channels[1].name,"Engineering Two");
    memset(channels[1].name,'x',sizeof(channels[1].name));
    check(!digit_interface_channels_valid(channels,2,2),
          "unterminated name rejected");
    strcpy(channels[1].name,"Engineering Two");
    channels[1].name[0]='\n';
    check(!digit_interface_channels_valid(channels,2,2),
          "control character in channel name rejected");
    strcpy(channels[1].name,"Engineering Two");
    channels[1].active=3;
    check(!digit_interface_channels_valid(channels,2,2),
          "nonboolean active state rejected");
    channels[1].active=0;
    check(digit_interface_channels_valid(channels,2,2),
          "inactive valid channel accepted");
    check(digit_interface_channel_exact_valid(&channels[1],1,"channel-2"),
          "exact inactive channel accepted");
    check(digit_interface_channels_valid(channels,2,2),
          "valid list recovers after malformed input");
    printf("Interface 1.4.10 milestone: %u executed, %u failed\n",
           executed,failed);
    return failed!=0;
}
