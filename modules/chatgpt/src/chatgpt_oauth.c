#include <string.h>
#include "chatgpt_oauth.h"
#include "chatgpt_auth.h"
#include <openssl/crypto.h>

/* Single OAuth code-to-bearer path. Codes are consumed by the PKCE
 * verifier; a redeemed code cannot mint a second access token. Authorization
 * and registered-client consent MUST happen prior to code issuance. */
int digit_chatgpt_oauth_exchange(digit_chatgpt_code_store_t *codes,
    digit_chatgpt_token_store_t *tokens,const char *code,
    const char *client_id,const char *redirect_uri,
    const char *verifier,const char *audience,time_t now,
    char token[DIGIT_CHATGPT_TOKEN_LENGTH])
{
    char principal[64]={0};
    int ok;
    if(token)token[0]=0;
    if(!token||!codes||!tokens||!audience||!audience[0])return 0;
    if(!digit_chatgpt_code_redeem(codes,code,client_id,redirect_uri,
                                  verifier,now,principal))return 0;
    ok=digit_chatgpt_token_issue(tokens,principal,audience,now,token);
    OPENSSL_cleanse(principal,sizeof(principal));
    return ok;
}


/* Single token-validation entry point for authenticated MCP requests.
 * A resolved identity still requires a separate current permission check. */
int digit_chatgpt_oauth_validate_mcp(const digit_chatgpt_token_store_t *tokens,
    const char *authorization_header,time_t now,char principal[64])
{
    char bearer[65]={0};
    int valid;
    if(principal)principal[0]=0;
    if(!principal||!tokens||
       !digit_chatgpt_bearer_header(authorization_header,bearer))
       return 0;
    valid=digit_chatgpt_token_resolve(tokens,bearer,"digit-mcp",now,principal);
    OPENSSL_cleanse(bearer,sizeof(bearer));
    return valid;
}

/* [AI:GPT-6 | 2026-10-11] Deny-by-default OAuth route dispatcher.
 * This must not issue codes or tokens until SA authorization,
 * PKCE state, registered client validation, and token binding exist. */
int digit_chatgpt_oauth_route(const digit_chatgpt_oauth_request_t *request,
                             digit_chatgpt_oauth_response_t *response)
{
    if (!request || !response ||
        !memchr(request->method, 0, sizeof(request->method)) ||
        !memchr(request->path, 0, sizeof(request->path)) ||
        !memchr(request->parameters, 0, sizeof(request->parameters)))
        return 0;
    memset(response, 0, sizeof(*response));
    if (strcmp(request->path, DIGIT_CHATGPT_OAUTH_AUTHORIZE) == 0) {
        if (strcmp(request->method, "GET") != 0) {
            response->status = 405;
            strcpy(response->body, "{\"error\":\"method_not_allowed\"}");
        } else {
            response->status = 503;
            strcpy(response->body, "{\"error\":\"temporarily_unavailable\",\"error_description\":\"SA-approved authorization flow not yet bound\"}");
        }
    } else if (strcmp(request->path, DIGIT_CHATGPT_OAUTH_TOKEN) == 0) {
        if (strcmp(request->method, "POST") != 0) {
            response->status = 405;
            strcpy(response->body, "{\"error\":\"method_not_allowed\"}");
        } else {
            response->status = 503;
            strcpy(response->body, "{\"error\":\"temporarily_unavailable\"}");
        }
    } else {
        response->status = 404;
        strcpy(response->body, "{\"error\":\"not_found\"}");
    }
    return 1;
}


/* Fixed-key approval is performed by Interface, which passes only the
 * independently verified ai.* principal. The secret never enters this module. */
static int oauth_unescape(const char *src,size_t len,char *dst,size_t cap){
 size_t i,j=0;
 if(!src||!dst||!cap)return 0;
 for(i=0;i<len;i++){
  unsigned char c=(unsigned char)src[i];
  if(c=='%'){
   int a,b;
   if(i+2>=len)return 0;
   a=src[++i];b=src[++i];
   a=a>='0'&&a<='9'?a-'0':a>='A'&&a<='F'?a-'A'+10:a>='a'&&a<='f'?a-'a'+10:-1;
   b=b>='0'&&b<='9'?b-'0':b>='A'&&b<='F'?b-'A'+10:b>='a'&&b<='f'?b-'a'+10:-1;
   if(a<0||b<0)return 0;
   c=(unsigned char)((a<<4)|b);
  }else if(c=='+')c=' ';
  if(c<32||c==127||j+1>=cap)return 0;
  dst[j++]=(char)c;
 }
 dst[j]=0;return j>0;
}
static int oauth_param(const char *form,const char *key,char *dst,size_t cap){
 size_t k=strlen(key),n=0;const char *p=form;int matches=0;
 dst[0]=0;
 if(!form||!memchr(form,0,2048))return 0;
 while(*p){
  const char *end=strchr(p,'&'),*eq=strchr(p,'=');
  if(!end)end=p+strlen(p);
  if(eq&&eq<end&&(size_t)(eq-p)==k&&!memcmp(p,key,k)){
   if(++matches>1||!oauth_unescape(eq+1,(size_t)(end-eq-1),dst,cap))return 0;
   n=1;
  }
  p=*end?end+1:end;
 }
 return n&&matches==1;
}
static int oauth_chatgpt_redirect(const char *uri){
 const char *hosts[]={"https://chatgpt.com/","https://chat.openai.com/"};
 size_t i;
 if(!uri||strlen(uri)>510||strchr(uri,'#')||strchr(uri,'\r')||strchr(uri,'\n'))return 0;
 for(i=0;i<2;i++)if(strncmp(uri,hosts[i],strlen(hosts[i]))==0)return 1;
 return 0;
}
static int oauth_safe_state(const char *state){
 const unsigned char *p=(const unsigned char*)state;size_t n=0;
 while(*p){if(!((*p>='A'&&*p<='Z')||(*p>='a'&&*p<='z')||
 (*p>='0'&&*p<='9')||*p=='_'||*p=='-'||*p=='.'||*p=='~'))return 0;
 p++;n++;if(n>128)return 0;}return n>0;
}
static int oauth_request_fields(const char *params,char *client,char *redirect,char *challenge,char *state){
 char method[16],type[32];
 return oauth_param(params,"response_type",type,sizeof(type))&&!strcmp(type,"code")&&
 oauth_param(params,"client_id",client,DIGIT_CHATGPT_CLIENT_MAX)&&
 oauth_param(params,"redirect_uri",redirect,DIGIT_CHATGPT_REDIRECT_MAX)&&
 oauth_chatgpt_redirect(redirect)&&
 oauth_param(params,"code_challenge_method",method,sizeof(method))&&!strcmp(method,"S256")&&
 oauth_param(params,"code_challenge",challenge,DIGIT_CHATGPT_CHALLENGE_SIZE)&&
 oauth_param(params,"state",state,129)&&oauth_safe_state(state);
}
int digit_chatgpt_oauth_route_with_stores(const digit_chatgpt_oauth_request_t *request,
 digit_chatgpt_oauth_response_t *response,digit_chatgpt_code_store_t *codes,
 digit_chatgpt_token_store_t *tokens){
 char client[128]={0},redirect[512]={0},challenge[44]={0},state[129]={0};
 if(!request||!response||!codes||!tokens||
 !memchr(request->method,0,sizeof(request->method))||
 !memchr(request->path,0,sizeof(request->path))||
 !memchr(request->parameters,0,sizeof(request->parameters))||
 !memchr(request->approved_principal,0,sizeof(request->approved_principal)))return 0;
 memset(response,0,sizeof(*response));
 if(!strcmp(request->path,"/oauth/authorize")&&!strcmp(request->method,"GET")){
  size_t i,j=0;char escaped[4096];
  if(!oauth_request_fields(request->parameters,client,redirect,challenge,state))
   goto bad_request;
  for(i=0;request->parameters[i];i++){
   char c=request->parameters[i];
   const char *entity=c=='&'?"&amp;":c=='"'?"&quot;":c=='<'?"&lt;":c=='>'?"&gt;":NULL;
   size_t n=entity?strlen(entity):1;
   if(j+n>=sizeof(escaped))goto bad_request;
   if(entity){memcpy(escaped+j,entity,n);j+=n;}else escaped[j++]=c;
  }escaped[j]=0;
  response->status=200;response->html=1;
  snprintf(response->body,sizeof(response->body),
   "<!doctype html><html><head><meta charset=\"utf-8\"><title>Digit connection approval</title></head>"
   "<body><h1>Authorize ChatGPT for Digit MCP</h1><p>Use the operator-approved Digit MCP key to approve this connection.</p>"
   "<form method=\"post\" action=\"/oauth/approve?%s\">"
   "<label>Digit MCP key <input type=\"password\" name=\"key\" required autocomplete=\"off\"></label>"
   "<button type=\"submit\">Authorize</button></form></body></html>",escaped);
  return 1;
 }
 if(!strcmp(request->path,"/oauth/approve")&&!strcmp(request->method,"POST")){
  char code[65]={0};
  if(!oauth_request_fields(request->parameters,client,redirect,challenge,state))
   goto bad_request;
  if(strncmp(request->approved_principal,"ai.",3)!=0){response->status=401;
   strcpy(response->body,"{\"error\":\"invalid_operator_key\"}");return 1;}
  if(!digit_chatgpt_code_issue(codes,request->approved_principal,client,redirect,
    challenge,time(NULL),code)){response->status=503;
    strcpy(response->body,"{\"error\":\"authorization_capacity_unavailable\"}");return 1;}
  if(snprintf(response->location,sizeof(response->location),"%s%ccode=%s&state=%s",
   redirect,strchr(redirect,'?')?'&':'?',code,state)>=(int)sizeof(response->location))
    goto bad_request;
  response->status=302;strcpy(response->body,"");return 1;
 }
 if(!strcmp(request->path,"/oauth/token")&&!strcmp(request->method,"POST")){
  char grant[64],code[65],verifier[129],token[65];
  if(!oauth_param(request->parameters,"grant_type",grant,sizeof(grant))||
     strcmp(grant,"authorization_code")||
     !oauth_param(request->parameters,"code",code,sizeof(code))||
     !oauth_param(request->parameters,"client_id",client,sizeof(client))||
     !oauth_param(request->parameters,"redirect_uri",redirect,sizeof(redirect))||
     !oauth_param(request->parameters,"code_verifier",verifier,sizeof(verifier)))
     goto bad_request;
  if(!digit_chatgpt_oauth_exchange(codes,tokens,code,client,redirect,
     verifier,"digit-mcp",time(NULL),token)){
   response->status=400;strcpy(response->body,"{\"error\":\"invalid_grant\"}");return 1;
  }
  response->status=200;
  snprintf(response->body,sizeof(response->body),
   "{\"access_token\":\"%s\",\"token_type\":\"Bearer\",\"expires_in\":3600}",token);
  OPENSSL_cleanse(token,sizeof(token));return 1;
 }
 return digit_chatgpt_oauth_route(request,response);
bad_request:
 response->status=400;strcpy(response->body,"{\"error\":\"invalid_request\"}");return 1;
}
