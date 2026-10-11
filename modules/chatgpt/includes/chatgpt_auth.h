#ifndef DIGIT_CHATGPT_AUTH_H
#define DIGIT_CHATGPT_AUTH_H
#include <stddef.h>

/* [AI:GPT-6 | 2026-10-11] ChatGPT-specific credentials validation helpers.
 * Authentication and access grants still require an authorized server binding. */
int digit_chatgpt_bearer_header(const char *value, char token[65]);
int digit_chatgpt_pkce_s256(const char *verifier, const char *challenge);
#endif
