#ifndef DIGIT_GRANT_STORE_H
#define DIGIT_GRANT_STORE_H
#include "grant_request.h"
/* [AI:GPT-6 | 2026-10-09] Protected Security-channel grant operation.
 * The caller's organization SA assignment is verified afresh. */
int digit_grant_security_scoped(const char *path,const char *root,const char *sa_registry,
 const char *actor,const digit_grant_request_t *request);
int digit_grant_alerts_scoped(const char *path,const char *root,const char *sa_registry,
 const char *actor,const digit_grant_request_t *request);
#endif
