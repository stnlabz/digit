#define _GNU_SOURCE
#include <string.h>
#include <stdint.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include "chatgpt_oauth_tokens.h"

/* Credential storage only. The caller must validate current SA grants,
 * AI identity and client audience at issue AND at every resolve operation.
 * No transport endpoint may infer permission from possession alone. */
static int bounded(const char *s,size_t cap)
{
    size_t n;
    if(!s)return 0;
    n=strnlen(s,cap);
    return n>0&&n<cap;
}
static int valid_hex(const char *s)
{
    size_t i;
    if(!s||strnlen(s,DIGIT_CHATGPT_TOKEN_LENGTH)!=64)return 0;
    for(i=0;i<64;i++)if(!((s[i]>='0'&&s[i]<='9')||
                         (s[i]>='a'&&s[i]<='f')))return 0;
    return 1;
}
void digit_chatgpt_tokens_init(digit_chatgpt_token_store_t *store)
{
    if(store)OPENSSL_cleanse(store,sizeof(*store));
}
int digit_chatgpt_token_issue(digit_chatgpt_token_store_t *store,
    const char *authorized_principal,const char *audience,
    time_t now,char token[DIGIT_CHATGPT_TOKEN_LENGTH])
{
    static const char hex[]="0123456789abcdef";
    unsigned char entropy[32],digest[SHA256_DIGEST_LENGTH];
    char generated[DIGIT_CHATGPT_TOKEN_LENGTH];
    size_t i,slot=DIGIT_CHATGPT_TOKEN_CAPACITY;
    if(token)token[0]=0;
    if(!store||!token||!bounded(authorized_principal,64)||
       strncmp(authorized_principal,"ai.",3)!=0||
       !bounded(audience,128)||now<=0||
       (int64_t)now>INT64_MAX-DIGIT_CHATGPT_TOKEN_TTL)return 0;
    for(i=0;i<DIGIT_CHATGPT_TOKEN_CAPACITY;i++){
        if(!store->entries[i].active||
           now>=store->entries[i].expires_at){slot=i;break;}
    }
    if(slot==DIGIT_CHATGPT_TOKEN_CAPACITY||
       RAND_bytes(entropy,sizeof(entropy))!=1)return 0;
    for(i=0;i<sizeof(entropy);i++){
        generated[2*i]=hex[entropy[i]>>4];
        generated[2*i+1]=hex[entropy[i]&15];
    }
    generated[64]=0;
    if(!SHA256((const unsigned char*)generated,64,digest)){
        OPENSSL_cleanse(entropy,sizeof(entropy));
        OPENSSL_cleanse(generated,sizeof(generated));
        return 0;
    }
    OPENSSL_cleanse(&store->entries[slot],sizeof(store->entries[slot]));
    memcpy(store->entries[slot].digest,digest,sizeof(digest));
    strcpy(store->entries[slot].principal,authorized_principal);
    strcpy(store->entries[slot].audience,audience);
    store->entries[slot].expires_at=now+DIGIT_CHATGPT_TOKEN_TTL;
    store->entries[slot].active=1;
    memcpy(token,generated,sizeof(generated));
    OPENSSL_cleanse(entropy,sizeof(entropy));
    OPENSSL_cleanse(digest,sizeof(digest));
    OPENSSL_cleanse(generated,sizeof(generated));
    return 1;
}
int digit_chatgpt_token_resolve(const digit_chatgpt_token_store_t *store,
    const char *token,const char *audience,time_t now,char principal[64])
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    size_t i;
    int found=0;
    if(principal)principal[0]=0;
    if(!store||!principal||!valid_hex(token)||
       !bounded(audience,128)||now<=0||
       !SHA256((const unsigned char*)token,64,digest))return 0;
    for(i=0;i<DIGIT_CHATGPT_TOKEN_CAPACITY;i++){
        const digit_chatgpt_token_entry_t *e=&store->entries[i];
        if(!e->active||now>=e->expires_at)continue;
        if(CRYPTO_memcmp(e->digest,digest,sizeof(digest))!=0)continue;
        if(strcmp(e->audience,audience)==0){
            strcpy(principal,e->principal);
            found=1;
        }
        break;
    }
    OPENSSL_cleanse(digest,sizeof(digest));
    return found;
}
int digit_chatgpt_token_revoke(digit_chatgpt_token_store_t *store,const char *token)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    size_t i;
    if(!store||!valid_hex(token)||
       !SHA256((const unsigned char*)token,64,digest))return 0;
    for(i=0;i<DIGIT_CHATGPT_TOKEN_CAPACITY;i++){
        digit_chatgpt_token_entry_t *e=&store->entries[i];
        if(e->active&&CRYPTO_memcmp(e->digest,digest,sizeof(digest))==0){
            OPENSSL_cleanse(e,sizeof(*e));
            OPENSSL_cleanse(digest,sizeof(digest));
            return 1;
        }
    }
    OPENSSL_cleanse(digest,sizeof(digest));
    return 0;
}
