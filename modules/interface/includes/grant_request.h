#ifndef DIGIT_GRANT_REQUEST_H
#define DIGIT_GRANT_REQUEST_H
/* [AI:GPT-6 | 2026-10-09] Scoped grant request validation for 1.6.0. */
typedef struct { char organization[64]; char project[64]; char channel[64]; char user[64]; } digit_grant_request_t;
int digit_grant_request_parse(const char *text, digit_grant_request_t *out);
#endif
