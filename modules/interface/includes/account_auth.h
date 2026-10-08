#ifndef DIGIT_ACCOUNT_AUTH_H
#define DIGIT_ACCOUNT_AUTH_H

#include <stddef.h>

/* [AI:GPT-6 | 2026-10-08]
 * On-disk records: user_id<TAB>active(0|1)<TAB>crypt_hash<NEWLINE>
 * This store contains hashes, never plaintext passwords. No registration API.
 * AUTH_FILE_PATH is managed by the human operator, not by Digit conversation.
 */
#define DIGIT_ACCOUNT_AUTH_PATH "/opt/digit/state/auth/accounts.tsv"
#define DIGIT_ACCOUNT_ID_MAX 64U
#define DIGIT_ACCOUNT_PASSWORD_MAX 256U

int digit_account_verify_file(const char *path, const char *user_id,
                              const char *password);
int digit_account_active_file(const char *path, const char *user_id);

#endif
