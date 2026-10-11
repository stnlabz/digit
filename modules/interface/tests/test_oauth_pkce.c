#include <stdio.h>
#include <string.h>
#include "oauth_pkce.h"

static int failures;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        ++failures;
    }
}

int main(void)
{
    /* RFC 7636 Appendix B test vector. */
    const char *verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const char *challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";
    char tampered[44];
    memcpy(tampered, challenge, sizeof(tampered));
    tampered[42] = tampered[42] == 'A' ? 'B' : 'A';
    check(digit_oauth_pkce_s256(verifier, challenge), "RFC7636 S256");
    check(!digit_oauth_pkce_s256(verifier, tampered), "incorrect challenge");
    check(!digit_oauth_pkce_s256("short", challenge), "short verifier");
    check(!digit_oauth_pkce_s256(verifier, "bad"), "short challenge");
    check(!digit_oauth_pkce_s256(NULL, challenge), "null verifier");
    check(!digit_oauth_pkce_s256(verifier, NULL), "null challenge");
    check(!digit_oauth_pkce_s256("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk!", challenge), "invalid verifier");
    check(!digit_oauth_pkce_s256(verifier, "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-c="), "padded challenge");
    if (!failures) puts("OAuth PKCE S256 checks passed");
    return failures ? 1 : 0;
}
