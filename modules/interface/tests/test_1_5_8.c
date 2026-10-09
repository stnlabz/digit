#include <stdio.h>
#include <string.h>
#include "channel_create_policy.h"
/* [AI:GPT-6 | 2026-10-09] 1.5.8 channel name boundary and negative cases. */
static unsigned total,fail;
static void t(int ok,const char *why){++total;if(ok)printf("PASS %02u - %s\n",total,why);else{++fail;printf("FAIL %02u - %s\n",total,why);}}
int main(void){
    char max[128],too_long[129];
    memset(max,'a',127);max[127]=0;
    memset(too_long,'a',128);too_long[128]=0;
    t(digit_interface_channel_create_name("general",128),"simple name");
    t(digit_interface_channel_create_name("Engineering Notes",128),"internal spaces");
    t(digit_interface_channel_create_name("a",128),"one-character name");
    t(digit_interface_channel_create_name(max,128),"maximum valid length");
    t(!digit_interface_channel_create_name(NULL,128),"null denied");
    t(!digit_interface_channel_create_name("",128),"empty denied");
    t(!digit_interface_channel_create_name(" ",128),"space-only denied");
    t(!digit_interface_channel_create_name(" name",128),"leading space denied");
    t(!digit_interface_channel_create_name("name ",128),"trailing space denied");
    t(!digit_interface_channel_create_name("bad\nname",128),"newline denied");
    t(!digit_interface_channel_create_name("bad\tname",128),"tab denied");
    t(!digit_interface_channel_create_name("bad\rname",128),"CR denied");
    t(!digit_interface_channel_create_name(too_long,128),"oversize denied");
    t(!digit_interface_channel_create_name("a",1),"undersized buffer denied");
    printf("Interface 1.5.8 milestone: %u executed, %u failed\n",total,fail);
    return fail?1:0;
}
