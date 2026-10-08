#ifndef DIGIT_PROJECT_ADMIN_ROUTE_H
#define DIGIT_PROJECT_ADMIN_ROUTE_H
#include <stddef.h>
#include <string.h>
#include "project_provision.h"

/* [AI:GPT-6 | 2026-10-08] Exact internal administration route parser.
 * No user-supplied actor, channel ID, grants or arbitrary Core service.
 * HTTP request must be authenticated before the caller invokes the bridge.
 */
static inline int digit_project_bind_route(const char *request,
    char *organization,size_t organization_capacity,
    char *project,size_t project_capacity)
{
    static const char prefix[]="POST /projects/";
    static const char suffix[]="/security/bind HTTP/1.1";
    const char *begin,*separator,*end;
    size_t a,b,i;
    if(!request||!organization||!project||
       organization_capacity==0||project_capacity==0)return 0;
    organization[0]=0;project[0]=0;
    if(strncmp(request,prefix,sizeof(prefix)-1)!=0)return 0;
    begin=request+sizeof(prefix)-1;
    separator=strchr(begin,'/');
    if(!separator)return 0;
    end=separator+1;
    while(*end && *end!='/')end++;
    a=(size_t)(separator-begin);
    b=(size_t)(end-(separator+1));
    if(a==0||b==0||a>=DIGIT_PROJECT_ID_MAX||b>=DIGIT_PROJECT_ID_MAX||
       a>=organization_capacity||b>=project_capacity||
       strcmp(end,suffix)!=0)return 0;
    for(i=0;i<a;i++){
        char c=begin[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
    }
    for(i=0;i<b;i++){
        char c=separator[1+i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
    }
    memcpy(organization,begin,a);organization[a]=0;
    memcpy(project,separator+1,b);project[b]=0;
    return 1;
}
#endif
