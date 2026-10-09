#include <string.h>
#include "channel_results.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.10: reject malformed
 * channel records, duplicate identities and invalid service counts. */
static int safe_id(const char *s,size_t capacity)
{
    size_t n=0,i;
    if(!s)return 0;
    while(n<capacity && s[n])++n;
    if(!n || n>=capacity)return 0;
    for(i=0;i<n;++i){
        unsigned char c=(unsigned char)s[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
    }
    return 1;
}
static int safe_name(const char *s,size_t capacity)
{
    size_t n=0;
    if(!s)return 0;
    while(n<capacity && s[n]){
        unsigned char c=(unsigned char)s[n++];
        if(c<32 || c==127)return 0;
    }
    return n>0 && n<capacity;
}
static int valid_record(const digit_channel_t *channel)
{
    return channel &&
           safe_id(channel->id,sizeof(channel->id)) &&
           safe_name(channel->name,sizeof(channel->name)) &&
           (channel->active==0 || channel->active==1);
}
int digit_interface_channels_valid(const digit_channel_t *channels,
                                   size_t count,size_t capacity)
{
    size_t i,j;
    if(count>capacity || (count && !channels))return 0;
    for(i=0;i<count;++i){
        if(!valid_record(&channels[i]))return 0;
        for(j=0;j<i;++j)
            if(strcmp(channels[i].id,channels[j].id)==0)return 0;
    }
    return 1;
}
int digit_interface_channel_exact_valid(const digit_channel_t *channel,
                                        int found,const char *requested_id)
{
    if(!safe_id(requested_id,DIGIT_CHANNEL_ID_MAX) ||
       (found!=0 && found!=1))return 0;
    if(!found)return 1;
    return valid_record(channel) &&
           strcmp(channel->id,requested_id)==0;
}
