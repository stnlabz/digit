#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/random.h>
#include "session_store.h"

/* [AI:GPT-6 | 2026-10-08] Single-owner bounded session state.
 * The Interface server currently processes requests on one accept thread.
 * If request processing becomes concurrent, callers must serialize access
 * or the session store must acquire internal synchronization.
 */
static int session_identity_valid(const char *identity)
{
    size_t n;
    if (identity == NULL) return 0;
    n = strnlen(identity, DIGIT_SESSION_ID_SIZE);
    return n > 0 && n < DIGIT_SESSION_ID_SIZE;
}

static int session_token_valid(const char *token)
{
    size_t i;
    if (token == NULL || strnlen(token, DIGIT_SESSION_TOKEN_SIZE) != 64U) return 0;
    for (i=0; i<64U; ++i) {
        char c=token[i];
        if (!((c>='0'&&c<='9') || (c>='a'&&c<='f'))) return 0;
    }
    return 1;
}

static int session_equal(const char *left, const char *right, size_t length)
{
    size_t i;
    unsigned int diff=0;
    for (i=0; i<length; ++i) diff |= (unsigned char)left[i] ^ (unsigned char)right[i];
    return diff == 0;
}

static int session_random(unsigned char bytes[32])
{
    size_t got=0;
    while (got<32U) {
        ssize_t result=getrandom(bytes+got,32U-got,0);
        if (result<0 && errno==EINTR) continue;
        if (result<=0) return 0;
        got+=(size_t)result;
    }
    return 1;
}

void digit_session_store_init(digit_session_store_t *store)
{
    if (store != NULL) memset(store,0,sizeof(*store));
}

int digit_session_issue(digit_session_store_t *store, const char *verified_identity,
                        int authentication_succeeded, time_t now,
                        char token[DIGIT_SESSION_TOKEN_SIZE])
{
    static const char digits[]="0123456789abcdef";
    unsigned char random_bytes[32];
    char generated[DIGIT_SESSION_TOKEN_SIZE];
    size_t i,slot=DIGIT_SESSION_CAPACITY;
    if (token != NULL) token[0]='\0';
    if (store==NULL || token==NULL || authentication_succeeded!=1 ||
        !session_identity_valid(verified_identity) || now<=0 ||
        now > (time_t)(INT64_MAX-DIGIT_SESSION_LIFETIME)) return 0;

    for (i=0; i<DIGIT_SESSION_CAPACITY; ++i)
        if (!store->entries[i].active || store->entries[i].expires_at<=now) {
            slot=i;break;
        }
    if (slot==DIGIT_SESSION_CAPACITY || !session_random(random_bytes)) return 0;

    for (i=0; i<32U; ++i) {
        generated[i*2U]=digits[random_bytes[i]>>4U];
        generated[i*2U+1U]=digits[random_bytes[i]&15U];
    }
    generated[64]='\0';
    memset(random_bytes,0,sizeof(random_bytes));
    memset(&store->entries[slot],0,sizeof(store->entries[slot]));
    memcpy(store->entries[slot].token,generated,sizeof(generated));
    memcpy(store->entries[slot].identity,verified_identity,strlen(verified_identity)+1U);
    store->entries[slot].expires_at=now+DIGIT_SESSION_LIFETIME;
    store->entries[slot].active=1;
    memcpy(token,generated,sizeof(generated));
    memset(generated,0,sizeof(generated));
    return 1;
}

int digit_session_resolve(const digit_session_store_t *store, const char *token,
                          time_t now, char *identity, size_t identity_size)
{
    size_t i;
    if (identity != NULL && identity_size>0) identity[0]='\0';
    if (store==NULL || !session_token_valid(token) || identity==NULL ||
        identity_size==0 || now<=0) return 0;
    for (i=0; i<DIGIT_SESSION_CAPACITY; ++i) {
        const digit_session_entry_t *entry=&store->entries[i];
        size_t length;
        if (!entry->active || now>=entry->expires_at) continue;
        if (!session_equal(token,entry->token,64U)) continue;
        length=strlen(entry->identity);
        if (length>=identity_size) return 0;
        memcpy(identity,entry->identity,length+1U);
        return 1;
    }
    return 0;
}

int digit_session_revoke(digit_session_store_t *store, const char *token)
{
    size_t i;
    if (store==NULL || !session_token_valid(token)) return 0;
    for (i=0; i<DIGIT_SESSION_CAPACITY; ++i)
        if (store->entries[i].active && session_equal(token,store->entries[i].token,64U)) {
            memset(&store->entries[i],0,sizeof(store->entries[i]));
            return 1;
        }
    return 0;
}

size_t digit_session_revoke_identity(digit_session_store_t *store, const char *identity)
{
    size_t i,count=0;
    if (store==NULL || !session_identity_valid(identity)) return 0;
    for (i=0; i<DIGIT_SESSION_CAPACITY; ++i)
        if (store->entries[i].active &&
            strcmp(store->entries[i].identity,identity)==0) {
            memset(&store->entries[i],0,sizeof(store->entries[i]));
            ++count;
        }
    return count;
}
