#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <string.h>
#include "service_registry.h"

/* [AI:GPT-6 | 2026-10-08] Protect service pointers from unload.
 * Transition ownership permits the loader to register/start services
 * while requests from other threads are refused. */
static digit_service_registry_t digit_services;
static int digit_services_initialized;
static pthread_mutex_t services_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t services_idle = PTHREAD_COND_INITIALIZER;
static size_t active_calls;
/* [AI:GPT-6 | 2026-10-08] Nested calls from already in-flight
 * handlers must be allowed to drain during a pending transition. */
static _Thread_local unsigned int invocation_depth;
static int transitioning;
static pthread_t transition_owner;

static void ensure_init(void)
{
    if (!digit_services_initialized)
    {
        memset(&digit_services, 0, sizeof(digit_services));
        digit_services_initialized = 1;
    }
}
static int owner(void)
{
    return transitioning && pthread_equal(pthread_self(), transition_owner);
}
void digit_service_registry_init(digit_service_registry_t *registry)
{
    if (registry) memset(registry, 0, sizeof(*registry));
}
int digit_service_transition_begin(void)
{
    pthread_mutex_lock(&services_lock);
    if (transitioning)
    {
        pthread_mutex_unlock(&services_lock);
        return 0;
    }
    transitioning = 1;
    transition_owner = pthread_self();
    while (active_calls)
        pthread_cond_wait(&services_idle, &services_lock);
    pthread_mutex_unlock(&services_lock);
    return 1;
}
void digit_service_transition_end(void)
{
    pthread_mutex_lock(&services_lock);
    if (owner())
    {
        transitioning = 0;
        pthread_cond_broadcast(&services_idle);
    }
    pthread_mutex_unlock(&services_lock);
}
int digit_service_register(const char *name, stnlabz_module_service_handler_fn handler, void *context)
{
    size_t i, length;
    int added = 0;
    if (!name || !handler) return 0;
    length = strnlen(name, STNLABZ_MODULE_SERVICE_NAME_MAX);
    if (!length || length >= STNLABZ_MODULE_SERVICE_NAME_MAX) return 0;
    pthread_mutex_lock(&services_lock);
    ensure_init();
    if (transitioning && !owner()) goto done;
    for (i = 0; i < DIGIT_SERVICE_MAX; ++i)
        if (digit_services.services[i].used &&
            strcmp(digit_services.services[i].name, name) == 0) goto done;
    for (i = 0; i < DIGIT_SERVICE_MAX; ++i)
    {
        digit_service_record_t *record = &digit_services.services[i];
        if (!record->used)
        {
            record->used = 1;
            memcpy(record->name, name, length + 1);
            record->handler = handler;
            record->handler_context = context;
            ++digit_services.count;
            added = 1;
            break;
        }
    }
done:
    pthread_mutex_unlock(&services_lock);
    return added;
}
int digit_service_unregister(const char *name, void *context)
{
    size_t i;
    int removed = 0;
    if (!name) return 0;
    pthread_mutex_lock(&services_lock);
    ensure_init();
    if (transitioning && !owner()) goto done;
    for (i = 0; i < DIGIT_SERVICE_MAX; ++i)
    {
        digit_service_record_t *record = &digit_services.services[i];
        if (record->used && strcmp(record->name, name) == 0 &&
            record->handler_context == context)
        {
            /* A module must only be unregistered after active service
             * calls drain. The transition owner provides that barrier. */
            while (active_calls && !transitioning)
                pthread_cond_wait(&services_idle, &services_lock);
            memset(record, 0, sizeof(*record));
            if (digit_services.count) --digit_services.count;
            removed = 1;
            break;
        }
    }
done:
    pthread_mutex_unlock(&services_lock);
    return removed;
}
stnlabz_module_result_t digit_service_invoke(const char *name,
    const void *request, size_t request_size, void *response,
    size_t response_size, size_t *response_used)
{
    size_t i;
    stnlabz_module_service_handler_fn handler = NULL;
    void *context = NULL;
    stnlabz_module_result_t result;
    if (!name || !response_used) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    pthread_mutex_lock(&services_lock);
    ensure_init();
    if (transitioning && !owner() && invocation_depth == 0) goto missing;
    for (i = 0; i < DIGIT_SERVICE_MAX; ++i)
    {
        if (digit_services.services[i].used &&
            strcmp(digit_services.services[i].name, name) == 0)
        {
            handler = digit_services.services[i].handler;
            context = digit_services.services[i].handler_context;
            ++active_calls;
            break;
        }
    }
    if (!handler) goto missing;
    pthread_mutex_unlock(&services_lock);
    ++invocation_depth;
    result = handler(request, request_size, response, response_size, response_used, context);
    --invocation_depth;
    pthread_mutex_lock(&services_lock);
    --active_calls;
    if (!active_calls) pthread_cond_broadcast(&services_idle);
    pthread_mutex_unlock(&services_lock);
    return result;
missing:
    pthread_mutex_unlock(&services_lock);
    return STNLABZ_MODULE_ERR_NOT_FOUND;
}
