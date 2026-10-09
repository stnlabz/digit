#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "interface_audit.h"

/* [AI:GPT-6 | 2026-10-08] Deliberately use a fixed route vocabulary:
 * no user, channel ID, URL parameter, token, or body may reach the log. */
static const char *operation(const char *request)
{
    if(!request)return "invalid_request";
    if(strncmp(request,"GET /health ",12)==0)return "health";
    if(strncmp(request,"POST /session/login ",20)==0)return "session_login";
    if(strncmp(request,"POST /session/logout ",21)==0)return "session_logout";
    if(strncmp(request,"GET /session ",13)==0)return "session_check";
    if(strncmp(request,"POST /projects/",15)==0)return "project_binding";
    if(strncmp(request,"GET /channels ",14)==0)return "channel_list";
    if(strncmp(request,"POST /channels ",15)==0)return "channel_provision";
    if(strncmp(request,"GET /channels/",14)==0||
       strncmp(request,"POST /channels/",15)==0)return "channel_access";
    if(strncmp(request,"GET /alerts",11)==0||
       strncmp(request,"POST /alerts/",13)==0)return "alert_access";
    if(strncmp(request,"POST /ask ",10)==0)return "request_dispatch";
    if(strncmp(request,"GET /corpus/",12)==0||
       strncmp(request,"POST /corpus/",13)==0)return "corpus_access";
    if(strncmp(request,"POST /input ",12)==0)return "corpus_input";
    if(strncmp(request,"POST /reason ",13)==0)return "reasoning";
    return "unknown_route";
}
int digit_interface_audit_format(const char *request,int status,
                                 char *output,size_t capacity)
{
    const char *outcome;
    int n;
    if(!output||capacity==0||status<100||status>599)return 0;
    output[0]=0;
    outcome=status>=500?"ERROR":status>=400?"REJECTED":"SUCCESS";
    n=snprintf(output,capacity,
               "[INTERFACE AUDIT] operation=%s outcome=%s status=%d",
               operation(request),outcome,status);
    if(n<0||(size_t)n>=capacity){output[0]=0;return 0;}
    return 1;
}
