#define _GNU_SOURCE
#include <string.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include "oauth_pkce.h"

/* [AI:GPT-6 | 2026-10-11] Reject malformed PKCE values before comparison.
 * RFC 7636 verifier: 43..128 unreserved ASCII characters; S256 challenge:
 * exactly 43 unpadded base64url characters. */
int digit_oauth_pkce_s256(const char *verifier, const char *challenge)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned char encoded[4 * ((SHA256_DIGEST_LENGTH + 2) / 3) + 1];
    size_t i, length;
    int ok;

    if (verifier == NULL || challenge == NULL)
        return 0;
    length = strnlen(verifier, 129);
    if (length < 43 || length > 128 || strnlen(challenge, 44) != 43)
        return 0;
    for (i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)verifier[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '.' ||
              c == '_' || c == '~'))
            return 0;
    }
    for (i = 0; i < 43; ++i) {
        unsigned char c = (unsigned char)challenge[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_'))
            return 0;
    }
    if (SHA256((const unsigned char *)verifier, length, digest) == NULL)
        return 0;
    if (EVP_EncodeBlock(encoded, digest, sizeof(digest)) != 44) {
        OPENSSL_cleanse(digest, sizeof(digest));
        return 0;
    }
    for (i = 0; i < 43; ++i) {
        if (encoded[i] == '+') encoded[i] = '-';
        else if (encoded[i] == '/') encoded[i] = '_';
    }
    ok = CRYPTO_memcmp(encoded, challenge, 43) == 0;
    OPENSSL_cleanse(digest, sizeof(digest));
    OPENSSL_cleanse(encoded, sizeof(encoded));
    return ok;
}
