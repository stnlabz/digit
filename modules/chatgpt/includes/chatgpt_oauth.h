#ifndef DIGIT_CHATGPT_OAUTH_H
#define DIGIT_CHATGPT_OAUTH_H
#include <stddef.h>
#define DIGIT_CHATGPT_OAUTH_SERVICE "chatgpt.oauth"
#define DIGIT_CHATGPT_OAUTH_PATH "/oauth"
#define DIGIT_CHATGPT_OAUTH_AUTHORIZE "/oauth/authorize"
#define DIGIT_CHATGPT_OAUTH_TOKEN "/oauth/token"
#define DIGIT_CHATGPT_OAUTH_REGISTER "/oauth/register"
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
int digit_chatgpt_oauth_route(const digit_chatgpt_oauth_request_t *request,
                             digit_chatgpt_oauth_response_t *response);
#endif
