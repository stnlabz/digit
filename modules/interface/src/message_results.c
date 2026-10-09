#include <string.h>
#include "message_results.h"
#include "controlled_communication.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.7: bound service replies
 * and check attribution and identity before rendering any message. */
static int component(const char *s,size_t n)
{
    size_t i=0;
    if(!s)return 0;
    while(i<n && s[i]){
        unsigned char c=(unsigned char)s[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'))return 0;
        ++i;
    }
    return i>0 && i<n;
}
int digit_interface_messages_valid(const digit_channel_message_t *messages,
                                   size_t count,size_t capacity,
                                   const char *channel_id)
{
    size_t i,j;
    if(!component(channel_id,DIGIT_CHANNEL_ID_MAX) ||
       count>capacity || (count && !messages))return 0;
    for(i=0;i<count;++i){
        const digit_channel_message_t *m=&messages[i];
        if(!component(m->id,sizeof(m->id)) ||
           !component(m->channel_id,sizeof(m->channel_id)) ||
           strcmp(m->channel_id,channel_id)!=0 ||
           !digit_controlled_message_valid(channel_id,m->origin,m->body))
            return 0;
        for(j=0;j<i;++j)
            if(strcmp(m->id,messages[j].id)==0)return 0;
    }
    return 1;
}
