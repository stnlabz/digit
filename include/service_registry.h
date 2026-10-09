#ifndef DIGIT_SERVICE_REGISTRY_H
#define DIGIT_SERVICE_REGISTRY_H

#include <stddef.h>
#include "module.h"

#define DIGIT_SERVICE_MAX 64

typedef struct
{
    int used;
    char name[STNLABZ_MODULE_SERVICE_NAME_MAX];
    stnlabz_module_service_handler_fn handler;
    void *handler_context;
} digit_service_record_t;

typedef struct
{
    digit_service_record_t services[DIGIT_SERVICE_MAX];
    size_t count;
} digit_service_registry_t;

void digit_service_registry_init(digit_service_registry_t *registry);
/* [AI:GPT-6 | 2026-10-08] Core-owned service transition barrier. */
int digit_service_transition_begin(void);
void digit_service_transition_end(void);
int digit_service_register(const char *name, stnlabz_module_service_handler_fn handler, void *handler_context);
int digit_service_unregister(const char *name, void *handler_context);
stnlabz_module_result_t digit_service_invoke(const char *name, const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used);

#endif
