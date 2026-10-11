#include <stdio.h>
#include <string.h>
#include "chatgpt_oauth_codes.h"
static int fail;
static void check(int ok,const char *label)
{
    if(!ok){ fprintf(stderr,"FAIL %s\n",label);fail++; }
}
int main(void)
{
    digit_chatgpt_code_store_t store;
    char code[DIGIT_CHATGPT_CODE_SIZE],identity[64];
    const char *verifier="dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const char *challenge="E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";
    digit_chatgpt_codes_init(&store);
    check(digit_chatgpt_code_issue(&store,"ai.chatgpt","registered",
       "https://client.example/callback",challenge,1000,code),"issue");
    check(!digit_chatgpt_code_redeem(&store,code,"other",
       "https://client.example/callback",verifier,1001,identity),"wrong client");
    check(!digit_chatgpt_code_redeem(&store,code,"registered",
       "https://client.example/callback",verifier,1001,identity),"burned");
    check(digit_chatgpt_code_issue(&store,"ai.chatgpt","registered",
       "https://client.example/callback",challenge,1000,code),"second issue");
    check(!digit_chatgpt_code_redeem(&store,code,"registered",
       "https://client.example/callback","incorrect-verifier",1001,identity),"wrong verifier");
    check(digit_chatgpt_code_issue(&store,"ai.chatgpt","registered",
       "https://client.example/callback",challenge,1000,code),"third issue");
    check(digit_chatgpt_code_redeem(&store,code,"registered",
       "https://client.example/callback",verifier,1001,identity) &&
       strcmp(identity,"ai.chatgpt")==0,"redeem correct");
    check(!digit_chatgpt_code_redeem(&store,code,"registered",
       "https://client.example/callback",verifier,1001,identity),"single use");
    check(digit_chatgpt_code_issue(&store,"ai.chatgpt","registered",
       "https://client.example/callback",challenge,1000,code),"fourth issue");
    check(!digit_chatgpt_code_redeem(&store,code,"registered",
       "https://client.example/callback",verifier,1300,identity),"expiry");
    check(!digit_chatgpt_code_issue(&store,"human","registered",
       "https://client.example/callback",challenge,1000,code),"non ai principal denied");
    digit_chatgpt_codes_init(&store);
    printf("ChatGPT OAuth code tests: %s\n",fail?"FAIL":"PASS");
    return fail?1:0;
}
