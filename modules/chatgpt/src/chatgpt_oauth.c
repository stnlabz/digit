#include <string.h>
#include "chatgpt_oauth.h"

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
