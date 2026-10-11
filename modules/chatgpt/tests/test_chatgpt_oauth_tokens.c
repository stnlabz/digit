#include <stdio.h>
#include <string.h>
#include "chatgpt_oauth_tokens.h"
static int failed=0;
static void check(int condition,const char *label){
    if(!condition){fprintf(stderr,"FAIL: %s\n",label);failed++;}
}
int main(void){
    digit_chatgpt_token_store_t store;
    char token[DIGIT_CHATGPT_TOKEN_LENGTH],principal[64],other[65];
    digit_chatgpt_tokens_init(&store);
    check(!digit_chatgpt_token_issue(&store,"human","digit-mcp",100,token),
          "reject non-AI principal");
    check(digit_chatgpt_token_issue(&store,"ai.chatgpt","digit-mcp",100,token),
          "issue AI token");
    check(strlen(token)==64,"64 hex token");
    check(digit_chatgpt_token_resolve(&store,token,"digit-mcp",101,principal)&&
          strcmp(principal,"ai.chatgpt")==0,"resolve bound identity");
    check(!digit_chatgpt_token_resolve(&store,token,"other",101,principal),
          "reject wrong audience");
    check(!digit_chatgpt_token_resolve(&store,token,"digit-mcp",3700,principal),
          "reject expired token");
    check(digit_chatgpt_token_revoke(&store,token),"revoke");
    check(!digit_chatgpt_token_resolve(&store,token,"digit-mcp",101,principal),
          "reject revoked token");
    check(digit_chatgpt_token_issue(&store,"ai.chatgpt","digit-mcp",200,token),
          "new token after revoke");
    memcpy(other,token,sizeof(other));
    other[0]=other[0]=='a'?'b':'a';
    check(!digit_chatgpt_token_resolve(&store,other,"digit-mcp",201,principal),
          "reject incorrect bearer");
    check(!digit_chatgpt_token_revoke(&store,other),"reject unknown revocation");
    digit_chatgpt_tokens_init(&store);
    printf("ChatGPT token lifecycle: %s\n",failed?"FAIL":"PASS");
    return failed?1:0;
}
