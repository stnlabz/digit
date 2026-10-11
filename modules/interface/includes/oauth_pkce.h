#ifndef DIGIT_OAUTH_PKCE_H
#define DIGIT_OAUTH_PKCE_H
#include <stddef.h>

/* [AI:GPT-6 | 2026-10-11] RFC 7636 S256 verification.
 * Strictly accepts base64url SHA-256 challenges and unreserved verifiers.
 * This helper does not issue tokens or grant authorization. */
int digit_oauth_pkce_s256(const char *verifier, const char *challenge);

#endif
