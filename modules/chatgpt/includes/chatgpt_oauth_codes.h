#ifndef DIGIT_CHATGPT_OAUTH_CODES_H
#define DIGIT_CHATGPT_OAUTH_CODES_H
#include <stddef.h>
#include <time.h>

/* In-memory, single-use OAuth authorization codes. No public issuance path.
 * Only trusted, independently authorized module code may call issue(). */
#define DIGIT_CHATGPT_CODE_SIZE 65
#define DIGIT_CHATGPT_CLIENT_MAX 128
#define DIGIT_CHATGPT_REDIRECT_MAX 512
#define DIGIT_CHATGPT_CHALLENGE_SIZE 44
#define DIGIT_CHATGPT_CODE_CAPACITY 32
#define DIGIT_CHATGPT_CODE_TTL 300
typedef struct {
    char code[DIGIT_CHATGPT_CODE_SIZE];
    char client_id[DIGIT_CHATGPT_CLIENT_MAX];
    char redirect_uri[DIGIT_CHATGPT_REDIRECT_MAX];
    char challenge[DIGIT_CHATGPT_CHALLENGE_SIZE];
    char principal[64];
    time_t expires_at;
    unsigned int active;
} digit_chatgpt_code_entry_t;
typedef struct {
    digit_chatgpt_code_entry_t entries[DIGIT_CHATGPT_CODE_CAPACITY];
} digit_chatgpt_code_store_t;
void digit_chatgpt_codes_init(digit_chatgpt_code_store_t *store);
int digit_chatgpt_code_issue(digit_chatgpt_code_store_t *store,
    const char *authorized_principal,const char *registered_client_id,
    const char *verified_redirect_uri,const char *s256_challenge,
    time_t now,char code[DIGIT_CHATGPT_CODE_SIZE]);
int digit_chatgpt_code_redeem(digit_chatgpt_code_store_t *store,
    const char *code,const char *client_id,const char *redirect_uri,
    const char *code_verifier,time_t now,char principal[64]);
#endif
