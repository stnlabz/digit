#include <string.h>

#include "service_registry.h"

static digit_service_registry_t digit_services;
static int digit_services_initialized = 0;

static void digit_service_ensure_init(void)
{
    if (!digit_services_initialized)
    {
        digit_service_registry_init(&digit_services);
        digit_services_initialized = 1;
    }
}

void digit_service_registry_init(digit_service_registry_t *registry)
{
    if (registry == NULL) return;
    memset(registry, 0, sizeof(*registry));
}

int digit_service_register(const char *name, stnlabz_module_service_handler_fn handler, void *handler_context)
{
    size_t index;
    size_t length;

    if (name == NULL || handler == NULL) return 0;
    length = strlen(name);
    if (length == 0 || length >= STNLABZ_MODULE_SERVICE_NAME_MAX) return 0;
    digit_service_ensure_init();

    for (index = 0; index < DIGIT_SERVICE_MAX; ++index)
        if (digit_services.services[index].used && strcmp(digit_services.services[index].name, name) == 0) return 0;

    for (index = 0; index < DIGIT_SERVICE_MAX; ++index)
    {
        digit_service_record_t *record = &digit_services.services[index];
        if (!record->used)
        {
            record->used = 1;
            memcpy(record->name, name, length + 1);
            record->handler = handler;
            record->handler_context = handler_context;
            ++digit_services.count;
            return 1;
        }
    }
    return 0;
}

int digit_service_unregister(const char *name, void *handler_context)
{
    size_t index;
    if (name == NULL) return 0;
    digit_service_ensure_init();
    for (index = 0; index < DIGIT_SERVICE_MAX; ++index)
    {
        digit_service_record_t *record = &digit_services.services[index];
        if (record->used && strcmp(record->name, name) == 0 && record->handler_context == handler_context)
        {
            memset(record, 0, sizeof(*record));
            if (digit_services.count > 0) --digit_services.count;
            return 1;
        }
    }
    return 0;
}

stnlabz_module_result_t digit_service_invoke(const char *name, const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used)
{
    size_t index;
    if (name == NULL || response_used == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    digit_service_ensure_init();
    for (index = 0; index < DIGIT_SERVICE_MAX; ++index)
    {
        digit_service_record_t *record = &digit_services.services[index];
        if (record->used && strcmp(record->name, name) == 0)
            return record->handler(request, request_size, response, response_size, response_used, record->handler_context);
    }
    return STNLABZ_MODULE_ERR_NOT_FOUND;
}
