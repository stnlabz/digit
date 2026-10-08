#ifndef DIGIT_INTERFACE_SESSION_STORE_H
#define DIGIT_INTERFACE_SESSION_STORE_H

#include <stddef.h>
#include <time.h>

/* [AI:GPT-6 | 2026-10-08] Bounded identity-bound session storage.
 * Tokens may be issued ONLY after successful server-side authentication.
 * Session state is never inferred from user-supplied conversation text.
 */
#define DIGIT_SESSION_CAPACITY 128U
#define DIGIT_SESSION_TOKEN_SIZE 65U
#define DIGIT_SESSION_ID_SIZE 64U
#define DIGIT_SESSION_LIFETIME 3600

typedef struct {
    char token[DIGIT_SESSION_TOKEN_SIZE];
    char identity[DIGIT_SESSION_ID_SIZE];
    time_t expires_at;
    int active;
} digit_session_entry_t;

typedef struct {
    digit_session_entry_t entries[DIGIT_SESSION_CAPACITY];
} digit_session_store_t;

void digit_session_store_init(digit_session_store_t *store);
int digit_session_issue(digit_session_store_t *store, const char *verified_identity,
                        int authentication_succeeded, time_t now,
                        char token[DIGIT_SESSION_TOKEN_SIZE]);
int digit_session_resolve(const digit_session_store_t *store, const char *token,
                          time_t now, char *identity, size_t identity_size);
int digit_session_revoke(digit_session_store_t *store, const char *token);
size_t digit_session_revoke_identity(digit_session_store_t *store, const char *identity);

#endif
