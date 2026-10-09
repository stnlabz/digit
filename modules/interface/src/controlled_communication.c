#include <stdio.h>
#include <string.h>
#include "controlled_communication.h"
#include "message_origin.h"

/* [AI:GPT-6 | 2026-10-08] 1.4.0: use Core's durable message
 * acknowledgement without inventing a second delivery authority. */
static int component(const char *v,size_t max)
{
    size_t n=0,i;
    if(!v)return 0;
    while(n<max && v[n])++n;
    if(!n||n>=max)return 0;
    for(i=0;i<n;++i)
        if(!((v[i]>='a'&&v[i]<='z')||
             (v[i]>='A'&&v[i]<='Z')||
             (v[i]>='0'&&v[i]<='9')||
             v[i]=='_'||v[i]=='-'))return 0;
    return 1;
}
int digit_controlled_message_valid(const char *channel,const char *origin,
                                   const char *body)
{
    char attributed[DIGIT_CHANNEL_ORIGIN_MAX];
    size_t length=0;
    if(!component(channel,DIGIT_CHANNEL_ID_MAX)||
       !digit_message_origin_from_identity(origin,attributed,sizeof(attributed))||
       !body)return 0;
    while(length<DIGIT_CHANNEL_MESSAGE_BODY_MAX && body[length])++length;
    return length>0 && length<DIGIT_CHANNEL_MESSAGE_BODY_MAX;
}
int digit_controlled_receipt(const digit_channel_message_t *message,
                             const char *channel,const char *origin,
                             char *output,size_t capacity)
{
    int n;
    if(!output||!capacity)return 0;
    output[0]=0;
    if(!message||!channel||!origin||
       !component(message->id,DIGIT_CHANNEL_MESSAGE_ID_MAX)||
       !component(channel,DIGIT_CHANNEL_ID_MAX)||
       !digit_controlled_message_valid(channel,origin,"x")||
       strcmp(message->channel_id,channel)!=0||
       strcmp(message->origin,origin)!=0)return 0;
    n=snprintf(output,capacity,
       "{\"message_id\":\"%s\",\"channel_id\":\"%s\",\"status\":\"persisted\"}",
       message->id,channel);
    if(n<0||(size_t)n>=capacity){output[0]=0;return 0;}
    return 1;
}
