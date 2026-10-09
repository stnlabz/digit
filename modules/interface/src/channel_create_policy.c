#include "channel_create_policy.h"
/* [AI:GPT-6 | 2026-10-09] Bound channel names; do not accept control
 * characters, leading/trailing spaces, or empty input. */
int digit_interface_channel_create_name(const char *name,size_t capacity)
{
    size_t n=0;
    if(!name||capacity<2)return 0;
    while(n<capacity && name[n]){
        unsigned char c=(unsigned char)name[n];
        if(c<32 || c==127)return 0;
        ++n;
    }
    if(n==0 || n>=capacity)return 0;
    if(name[0]==' '||name[n-1]==' ')return 0;
    return 1;
}
