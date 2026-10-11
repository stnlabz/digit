#define _GNU_SOURCE
#include <string.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include "chatgpt_auth.h"

/* [AI:GPT-6 | 2026-10-11] Parse only the existing 64-hex session bearer format.
 * Caller must first isolate a unique Authorization header from HTTP framing.
 * This function validates syntax, NOT credential validity or privilege. */
int digit_chatgpt_bearer_header(const char *value, char token[65])
{
    size_t i;
    if (!token) return 0;
    token[0] = 0;
    if (!value || strnlen(value, 73) != 71 ||
        memcmp(value, "Bearer ", 7) != 0) return 0;
    for (i = 0; i < 64; ++i) {
        unsigned char c = (unsigned char)value[7+i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return 0;
    }
    memcpy(token, value+7, 64);
    token[64] = 0;
    return 1;
}

/* [AI:GPT-6 | 2026-10-11] RFC 7636: SHA-256 PKCE challenge comparison.
 * No plain-method fallback; reject invalid verifier and encoded challenge. */
int digit_chatgpt_pkce_s256(const char *verifier, const char *challenge)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned char encoded[45];
    size_t i, n;
    int valid;
    if (!verifier || !challenge) return 0;
    n = strnlen(verifier, 129);
    if (n < 43 || n > 128 || strnlen(challenge, 44) != 43) return 0;
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)verifier[i];
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '-' || c == '.' || c == '_' || c == '~')) return 0;
    }
    for (i = 0; i < 43; ++i) {
        unsigned char c = (unsigned char)challenge[i];
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) return 0;
    }
    if (!SHA256((const unsigned char *)verifier, n, digest)) return 0;
    if (EVP_EncodeBlock(encoded, digest, sizeof(digest)) != 44) {
        OPENSSL_cleanse(digest, sizeof(digest));
        return 0;
    }
    for (i = 0; i < 43; ++i) {
        if (encoded[i] == '+') encoded[i] = '-';
        else if (encoded[i] == '/') encoded[i] = '_';
    }
    valid = CRYPTO_memcmp(encoded, challenge, 43) == 0;
    OPENSSL_cleanse(encoded, sizeof(encoded));
    OPENSSL_cleanse(digest, sizeof(digest));
    return valid;
}
