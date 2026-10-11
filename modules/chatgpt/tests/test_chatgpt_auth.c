#include <stdio.h>
#include <string.h>
#include "chatgpt_auth.h"

static int failures;
static void check(int success, const char *name)
{
    if (!success) { fprintf(stderr, "FAIL %s\n", name); ++failures; }
}
int main(void)
{
    /* RFC 7636 Appendix B canonical S256 test vector. */
    const char *verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const char *challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";
    char header[72], token[65], tampered[44];
    memset(header, 'a', sizeof(header));
    memcpy(header, "Bearer ", 7);
    header[71] = 0;
    check(digit_chatgpt_bearer_header(header, token), "valid bearer syntax");
    check(strlen(token) == 64, "token copy");
    check(!digit_chatgpt_bearer_header("Basic abc", token), "reject scheme");
    check(!digit_chatgpt_bearer_header("Bearer short", token), "reject length");
    header[7] = 'G';
    check(!digit_chatgpt_bearer_header(header, token), "reject nonhex");
    check(token[0] == 0, "clear invalid token");
    check(!digit_chatgpt_bearer_header(NULL, token), "reject null header");
    check(digit_chatgpt_pkce_s256(verifier, challenge), "RFC7636 S256");
    memcpy(tampered, challenge, sizeof(tampered));
    tampered[42] = tampered[42] == 'A' ? 'B' : 'A';
    check(!digit_chatgpt_pkce_s256(verifier, tampered), "reject wrong S256");
    check(!digit_chatgpt_pkce_s256("short", challenge), "reject short verifier");
    check(!digit_chatgpt_pkce_s256(verifier, "short"), "reject short challenge");
    check(!digit_chatgpt_pkce_s256(NULL, challenge), "reject null verifier");
    check(!digit_chatgpt_pkce_s256(verifier, NULL), "reject null challenge");
    if (!failures) puts("ChatGPT credential helper tests PASS");
    return failures != 0;
}
