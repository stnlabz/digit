#ifndef DIGIT_MESSAGE_ORIGIN_H
#define DIGIT_MESSAGE_ORIGIN_H
#include <stddef.h>
#include <string.h>
#include "channel.h"

/* [AI:GPT-6 | 2026-10-08]
 * The message author is the authenticated session identity.
 * Reject identities that cannot fit Core's existing origin field.
 */
static inline int digit_message_origin_from_identity(const char *identity,
                                                       char *out,size_t capacity)
{
    size_t n,i;
    if(!identity||!out||capacity<DIGIT_CHANNEL_ORIGIN_MAX)return 0;
    n=strnlen(identity,DIGIT_CHANNEL_ORIGIN_MAX);
    if(n==0||n>=DIGIT_CHANNEL_ORIGIN_MAX)return 0;
    for(i=0;i<n;i++){
        char c=identity[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;
    }
    memcpy(out,identity,n+1);
    return 1;
}
#endif
