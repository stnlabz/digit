#ifndef DIGIT_CHATGPT_OAUTH_TOKENS_H
#define DIGIT_CHATGPT_OAUTH_TOKENS_H
#include <time.h>
#define DIGIT_CHATGPT_TOKEN_LENGTH 65
#define DIGIT_CHATGPT_TOKEN_CAPACITY 32
#define DIGIT_CHATGPT_TOKEN_TTL 3600
typedef struct {
    unsigned char digest[32];
    char principal[64];
    char audience[128];
    time_t expires_at;
    unsigned int active;
} digit_chatgpt_token_entry_t;
typedef struct {
    digit_chatgpt_token_entry_t entries[DIGIT_CHATGPT_TOKEN_CAPACITY];
} digit_chatgpt_token_store_t;
void digit_chatgpt_tokens_init(digit_chatgpt_token_store_t *store);
int digit_chatgpt_token_issue(digit_chatgpt_token_store_t *store,
    const char *authorized_principal, const char *audience,
    time_t now,char token[DIGIT_CHATGPT_TOKEN_LENGTH]);
int digit_chatgpt_token_resolve(const digit_chatgpt_token_store_t *store,
    const char *token,const char *audience,time_t now,char principal[64]);
int digit_chatgpt_token_revoke(digit_chatgpt_token_store_t *store,
    const char *token);
#endif
