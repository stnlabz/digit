#ifndef DIGIT_CHATGPT_OAUTH_H
#define DIGIT_CHATGPT_OAUTH_H
#include <stddef.h>
#include <time.h>
#include "chatgpt_oauth_codes.h"
#include "chatgpt_oauth_tokens.h"
#define DIGIT_CHATGPT_OAUTH_SERVICE "chatgpt.oauth"
#define DIGIT_CHATGPT_OAUTH_VALIDATE_SERVICE "chatgpt.oauth.validate"
#define DIGIT_CHATGPT_OAUTH_PATH "/oauth"
#define DIGIT_CHATGPT_OAUTH_AUTHORIZE "/oauth/authorize"
#define DIGIT_CHATGPT_OAUTH_TOKEN "/oauth/token"
/* ChatGPT OAuth route adapter contract: does not own the public HTTP listener.
 * Transport must supply a validated method and exact path; no raw credentials. */
typedef struct {
    char method[8];
    char path[96];
} digit_chatgpt_oauth_request_t;
typedef struct {
    unsigned int status;
    char body[1024];
} digit_chatgpt_oauth_response_t;
/* Internal code-to-token exchange. The authorization code MUST have been
 * created following SA approval, registered-client checks and consent.
 * Never expose this function as an unauthenticated endpoint. */
int digit_chatgpt_oauth_exchange(digit_chatgpt_code_store_t *codes,
    digit_chatgpt_token_store_t *tokens, const char *code,
    const char *client_id,const char *redirect_uri,
    const char *verifier,const char *audience,time_t now,
    char token[DIGIT_CHATGPT_TOKEN_LENGTH]);
/* Validates the one OAuth bearer issued by the PKCE exchange.
 * This does not replace current SA permission checks or HTTP authentication.
 * The MCP transport must call this before executing an authorized request. */
int digit_chatgpt_oauth_validate_mcp(const digit_chatgpt_token_store_t *tokens,
    const char *authorization_header,time_t now,char principal[64]);
typedef struct {
    char authorization[80];
} digit_chatgpt_oauth_validate_request_t;
typedef struct {
    char principal[64];
    unsigned int authenticated;
} digit_chatgpt_oauth_validate_response_t;
int digit_chatgpt_oauth_route(const digit_chatgpt_oauth_request_t *request,
                             digit_chatgpt_oauth_response_t *response);
#endif
