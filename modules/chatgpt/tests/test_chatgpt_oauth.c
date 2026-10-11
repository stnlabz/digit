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
    {
        digit_chatgpt_code_store_t codes;
        digit_chatgpt_token_store_t tokens;
        char code[65],token[65],principal[64];
        const char *v="dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
        const char *c="E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";
        digit_chatgpt_codes_init(&codes);
        digit_chatgpt_tokens_init(&tokens);
        ok &= digit_chatgpt_code_issue(&codes,"ai.chatgpt","client",
          "https://client.example/callback",c,1000,code);
        ok &= digit_chatgpt_oauth_exchange(&codes,&tokens,code,"client",
          "https://client.example/callback",v,"digit-mcp",1001,token);
        ok &= digit_chatgpt_token_resolve(&tokens,token,"digit-mcp",1002,
          principal) && strcmp(principal,"ai.chatgpt")==0;
        ok &= !digit_chatgpt_oauth_exchange(&codes,&tokens,code,"client",
          "https://client.example/callback",v,"digit-mcp",1002,token);
    }
    printf("ChatGPT OAuth route tests: %s\n",ok?"PASS":"FAIL");
    return ok?0:1;
}
