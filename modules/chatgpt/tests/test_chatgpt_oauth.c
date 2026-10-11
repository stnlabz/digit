#include <stdio.h>
#include <string.h>
#include "chatgpt_oauth.h"
int main(void)
{
    digit_chatgpt_oauth_request_t req={{0}};
    digit_chatgpt_oauth_response_t out={0};
    int ok=1;
    strcpy(req.method,"GET");
    strcpy(req.path,"/oauth/authorize");
    ok &= digit_chatgpt_oauth_route(&req,&out) && out.status==503;
    strcpy(req.path,"/oauth/token");
    ok &= digit_chatgpt_oauth_route(&req,&out) && out.status==405;
    strcpy(req.method,"POST");
    ok &= digit_chatgpt_oauth_route(&req,&out) && out.status==503;
    strcpy(req.path,"/oauth/register");
    ok &= digit_chatgpt_oauth_route(&req,&out) && out.status==404;
    strcpy(req.path,"/oauth/unknown");
    ok &= digit_chatgpt_oauth_route(&req,&out) && out.status==404;
    memset(&req,'X',sizeof(req));
    ok &= !digit_chatgpt_oauth_route(&req,&out);
    printf("ChatGPT OAuth route tests: %s\n",ok?"PASS":"FAIL");
    return ok?0:1;
}
