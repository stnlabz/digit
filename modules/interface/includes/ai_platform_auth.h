#ifndef DIGIT_AI_PLATFORM_AUTH_H
#define DIGIT_AI_PLATFORM_AUTH_H
#include <stddef.h>
/* Operator-maintained, private records:
 * identity<TAB>active<TAB>dm<TAB>diagnostics<TAB>memory_write<TAB>learning<TAB>developer
 * Each capability must be 0 or 1. Developer is descriptive, not SA authority. */
#define DIGIT_AI_PLATFORM_REGISTRY "/opt/digit/state/auth/ai_platform_permissions.tsv"
#define DIGIT_AI_PLATFORM_CREDENTIALS "/opt/digit/state/auth/ai_platform_accounts.tsv"
typedef struct {
 unsigned int active,dm,diagnostics,memory_write,learning,developer;
} digit_ai_platform_permissions_t;
/* 1 = exact, unique, valid record; 0 = deny. */
int digit_ai_platform_permissions(const char *path,const char *identity,
                                  digit_ai_platform_permissions_t *out);
#endif
