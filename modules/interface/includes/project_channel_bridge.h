#ifndef DIGIT_PROJECT_CHANNEL_BRIDGE_H
#define DIGIT_PROJECT_CHANNEL_BRIDGE_H

#include <stddef.h>
#include "core_services.h"

/* [AI:GPT-6 | 2026-10-08] Internal Core-channel binding.
 * External callers must supply the qualified Core service adapter.
 * A project is not channel-connected until its private binding is persisted.
 * HTTP has no project-create or membership-management route.
 */
typedef int (*digit_project_core_invoke_t)(const char *service,
    const void *request,size_t request_size,void *response,
    size_t response_size,size_t *response_used,void *context);

int digit_project_bind_security(const char *root,const char *organization,
    const char *project,digit_project_core_invoke_t invoke,void *context);

int digit_project_security_channel_id(const char *root,const char *organization,
    const char *project,char *channel_id,size_t capacity);

#endif
