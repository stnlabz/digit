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
        !memchr(request->path, 0, sizeof(request->path)))
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
