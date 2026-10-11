#define _GNU_SOURCE
#include <string.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include "chatgpt_oauth_codes.h"
#include "chatgpt_auth.h"

/* This library does not authenticate operators or clients, authorize consent,
 * register redirect URIs, or issue bearer tokens. Those checks must precede
 * issue(); no network route may call this primitive directly. */
static int bounded(const char *s,size_t cap)
{
    size_t n;
    if (!s) return 0;
    n=strnlen(s,cap);
    return n>0 && n<cap;
}
static int code_hex(const char *s)
{
    size_t i;
    if (!s || strnlen(s,DIGIT_CHATGPT_CODE_SIZE)!=64) return 0;
    for(i=0;i<64;i++) if(!((s[i]>='0'&&s[i]<='9')||
                             (s[i]>='a'&&s[i]<='f'))) return 0;
    return 1;
}
static int code_equal(const char *a,const char *b)
{
    return CRYPTO_memcmp(a,b,64)==0;
}
void digit_chatgpt_codes_init(digit_chatgpt_code_store_t *store)
{
    if(store) OPENSSL_cleanse(store,sizeof(*store));
}
int digit_chatgpt_code_issue(digit_chatgpt_code_store_t *store,
    const char *authorized_principal,const char *registered_client_id,
    const char *verified_redirect_uri,const char *s256_challenge,
    time_t now,char code[DIGIT_CHATGPT_CODE_SIZE])
{
    unsigned char random[32];
    static const char hex[]="0123456789abcdef";
    char generated[DIGIT_CHATGPT_CODE_SIZE];
    size_t slot=DIGIT_CHATGPT_CODE_CAPACITY,i;
    if(code) code[0]=0;
    if(!store||!code||now<=0||
       !bounded(authorized_principal,64)||
       strncmp(authorized_principal,"ai.",3)!=0||
       !bounded(registered_client_id,DIGIT_CHATGPT_CLIENT_MAX)||
       !bounded(verified_redirect_uri,DIGIT_CHATGPT_REDIRECT_MAX)||
       !bounded(s256_challenge,DIGIT_CHATGPT_CHALLENGE_SIZE)||
       strlen(s256_challenge)!=43 ||
       !digit_chatgpt_pkce_s256(
          "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk",
          "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM") ||
       now>(time_t)(2147483647-DIGIT_CHATGPT_CODE_TTL)) return 0;
    for(i=0;i<43;i++) {
        unsigned char c=(unsigned char)s256_challenge[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='_'||c=='-')) return 0;
    }
    for(i=0;i<DIGIT_CHATGPT_CODE_CAPACITY;i++)
        if(!store->entries[i].active || store->entries[i].expires_at<=now){
            slot=i;break;
        }
    if(slot==DIGIT_CHATGPT_CODE_CAPACITY || RAND_bytes(random,sizeof(random))!=1)
        return 0;
    for(i=0;i<32;i++){
        generated[2*i]=hex[random[i]>>4];
        generated[2*i+1]=hex[random[i]&15];
    }
    generated[64]=0;
    OPENSSL_cleanse(random,sizeof(random));
    OPENSSL_cleanse(&store->entries[slot],sizeof(store->entries[slot]));
    {
        digit_chatgpt_code_entry_t *e=&store->entries[slot];
        memcpy(e->code,generated,sizeof(generated));
        strcpy(e->client_id,registered_client_id);
        strcpy(e->redirect_uri,verified_redirect_uri);
        strcpy(e->challenge,s256_challenge);
        strcpy(e->principal,authorized_principal);
        e->expires_at=now+DIGIT_CHATGPT_CODE_TTL;
        e->active=1;
    }
    memcpy(code,generated,sizeof(generated));
    OPENSSL_cleanse(generated,sizeof(generated));
    return 1;
}
int digit_chatgpt_code_redeem(digit_chatgpt_code_store_t *store,
    const char *code,const char *client_id,const char *redirect_uri,
    const char *code_verifier,time_t now,char principal[64])
{
    size_t i;
    if(principal) principal[0]=0;
    if(!store||!principal||!code_hex(code)||
       !bounded(client_id,DIGIT_CHATGPT_CLIENT_MAX)||
       !bounded(redirect_uri,DIGIT_CHATGPT_REDIRECT_MAX)||
       !bounded(code_verifier,129)||now<=0) return 0;
    for(i=0;i<DIGIT_CHATGPT_CODE_CAPACITY;i++){
        digit_chatgpt_code_entry_t *e=&store->entries[i];
        if(!e->active||!code_equal(e->code,code)) continue;
        /* Burn the matching code on every redemption attempt, including
         * mismatched clients/verifiers and expired codes. */
        if(now<e->expires_at &&
           strcmp(e->client_id,client_id)==0 &&
           strcmp(e->redirect_uri,redirect_uri)==0 &&
           digit_chatgpt_pkce_s256(code_verifier,e->challenge)){
            memcpy(principal,e->principal,strlen(e->principal)+1);
            OPENSSL_cleanse(e,sizeof(*e));
            return 1;
        }
        OPENSSL_cleanse(e,sizeof(*e));
        return 0;
    }
    return 0;
}
