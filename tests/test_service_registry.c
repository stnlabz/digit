#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "service_registry.h"

/* [AI:GPT-6 | 2026-10-08] Verify in-flight calls finish before
 * hotload transition acquires exclusive service dispatch. */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static int entered, release_call, transition_done, failures, tests;
#define CHECK(ok, name) do { ++tests; if (ok) printf("PASS %02d - %s\n",tests,name); else { printf("FAIL %02d - %s\n",tests,name); ++failures; } } while (0)

static stnlabz_module_result_t handler(const void *in,size_t is,void *out,size_t os,size_t *used,void *ctx)
{
    (void)in;(void)is;(void)out;(void)os;(void)ctx;
    pthread_mutex_lock(&lock);
    entered = 1;
    pthread_cond_broadcast(&changed);
    while (!release_call) pthread_cond_wait(&changed,&lock);
    pthread_mutex_unlock(&lock);
    *used = 0;
    return STNLABZ_MODULE_OK;
}
static void *invoke_thread(void *arg)
{
    size_t used = 0;
    stnlabz_module_result_t *result = arg;
    *result = digit_service_invoke("test.inflight",NULL,0,NULL,0,&used);
    return NULL;
}
static void *transition_thread(void *arg)
{
    (void)arg;
    if (digit_service_transition_begin())
    {
        pthread_mutex_lock(&lock);
        transition_done = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&lock);
        digit_service_transition_end();
    }
    return NULL;
}
/* [AI:GPT-6 | 2026-10-08] The module graph legitimately
 * chains services; nested dispatch must remain available. */
static stnlabz_module_result_t nested_leaf(const void *in,size_t is,void *out,size_t os,size_t *used,void *ctx)
{
    (void)in;(void)is;(void)out;(void)os;(void)ctx;
    *used = 0;
    return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t nested_parent(const void *in,size_t is,void *out,size_t os,size_t *used,void *ctx)
{
    (void)in;(void)is;(void)out;(void)os;(void)ctx;
    return digit_service_invoke("test.leaf", NULL, 0, NULL, 0, used);
}
int main(void)
{
    pthread_t caller, swapper;
    stnlabz_module_result_t call_result = STNLABZ_MODULE_ERR_NOT_FOUND;
    int begun = 0, swapping = 0;
    CHECK(digit_service_register("test.leaf",nested_leaf,NULL), "nested leaf registered");
    CHECK(digit_service_register("test.parent",nested_parent,NULL), "nested parent registered");
    { size_t used=1;
      CHECK(digit_service_invoke("test.parent",NULL,0,NULL,0,&used)==STNLABZ_MODULE_OK && used==0,
            "nested service invocation completes"); }
    CHECK(digit_service_unregister("test.parent",NULL), "nested parent unregistered");
    CHECK(digit_service_unregister("test.leaf",NULL), "nested leaf unregistered");
    CHECK(digit_service_register("test.inflight",handler,NULL), "service registered");
    if (!pthread_create(&caller,NULL,invoke_thread,&call_result)) begun = 1;
    CHECK(begun, "in-flight call thread started");
    if (!begun) return 1;
    pthread_mutex_lock(&lock);
    while (!entered) pthread_cond_wait(&changed,&lock);
    pthread_mutex_unlock(&lock);
    if (!pthread_create(&swapper,NULL,transition_thread,NULL)) swapping = 1;
    CHECK(swapping, "transition thread started");
    if (!swapping) return 1;
    pthread_mutex_lock(&lock);
    CHECK(!transition_done, "transition does not preempt active handler");
    release_call = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    pthread_join(caller,NULL);
    pthread_join(swapper,NULL);
    CHECK(call_result == STNLABZ_MODULE_OK, "in-flight handler completed");
    CHECK(transition_done, "transition acquired after drain");
    CHECK(digit_service_unregister("test.inflight",NULL), "service safely unregistered");
    printf("Service registry: %d executed, %d failed\n",tests,failures);
    return failures != 0;
}
