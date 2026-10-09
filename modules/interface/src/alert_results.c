#include <string.h>
#include "alert_results.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.8: fail closed on
 * malformed or oversized Core alert lists without partial output. */
static int bounded_text(const char *s,size_t capacity,int require_content)
{
    size_t n=0;
    if(!s)return 0;
    while(n<capacity && s[n]){
        unsigned char c=(unsigned char)s[n++];
        if(c<32 || c==127)return 0;
    }
    return n<capacity && (!require_content || n>0);
}
static int safe_id(const char *id,size_t capacity)
{
    size_t i,n=0;
    if(!id)return 0;
    while(n<capacity && id[n])++n;
    if(!n || n>=capacity)return 0;
    for(i=0;i<n;++i){
        unsigned char c=(unsigned char)id[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
    }
    return 1;
}
int digit_interface_alerts_valid(const digit_alert_t *alerts,size_t count,
                                 size_t capacity,int unacknowledged_only)
{
    size_t i,j;
    if((unacknowledged_only!=0 && unacknowledged_only!=1) ||
       count>capacity || (count && !alerts))return 0;
    for(i=0;i<count;++i){
        const digit_alert_t *a=&alerts[i];
        if(!safe_id(a->id,sizeof(a->id)) ||
           !bounded_text(a->source,sizeof(a->source),1) ||
           !bounded_text(a->summary,sizeof(a->summary),1) ||
           !bounded_text(a->detail,sizeof(a->detail),0) ||
           !bounded_text(a->operational_state,sizeof(a->operational_state),1) ||
           a->severity<DIGIT_ALERT_INFO || a->severity>DIGIT_ALERT_CRITICAL ||
           (a->acknowledged!=0 && a->acknowledged!=1) ||
           (unacknowledged_only && a->acknowledged))
            return 0;
        for(j=0;j<i;++j)
            if(strcmp(a->id,alerts[j].id)==0)return 0;
    }
    return 1;
}
