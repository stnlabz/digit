#ifndef DIGIT_INTERFACE_HTTP_AUTH_H
#define DIGIT_INTERFACE_HTTP_AUTH_H

#include <stddef.h>
#include <time.h>
#include "session_store.h"

/* [AI:GPT-6 | 2026-10-08] Strict Bearer extraction from a complete HTTP
 * request header. Reject duplicate, malformed, or ambiguous credentials.
 * A successful parse is NOT authentication; session resolution is required.
 */
int digit_http_bearer_token(const char *request, char token[DIGIT_SESSION_TOKEN_SIZE]);
int digit_http_resolve_identity(const digit_session_store_t *sessions,
                                const char *request, time_t now,
                                char *identity, size_t identity_size);
#endif
