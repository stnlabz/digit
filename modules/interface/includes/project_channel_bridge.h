#ifndef DIGIT_PROJECT_CHANNEL_BRIDGE_H
#define DIGIT_PROJECT_CHANNEL_BRIDGE_H

#include <stddef.h>
#include "core_services.h"
#include "module.h"

/* [AI:GPT-6 | 2026-10-08] Internal Core-channel binding.
 * External callers must supply the qualified Core service adapter.
 * A project is not channel-connected until its private binding is persisted.
 * HTTP has no project-create or membership-management route.
 */
typedef int (*digit_project_core_invoke_t)(const char *service,
    const void *request,size_t request_size,void *response,
    size_t response_size,size_t *response_used,void *context);

/* Internal trusted operation: verified SA and project #security membership
 * are required before any Core service is invoked. No HTTP route is exposed. */
int digit_project_bind_security(const char *root,const char *organization,
    const char *project,const char *actor,const char *sa_registry,
    digit_project_core_invoke_t invoke,void *context);

/* Trusted module-host adapter. This accepts only a host service callback,
 * never a caller-supplied channel ID, grant, or HTTP request. The underlying
 * bridge independently verifies current SA status and project membership. */
int digit_project_bind_security_host(const char *root,const char *organization,
    const char *project,const char *actor,const char *sa_registry,
    const stnlabz_module_host_t *host);

int digit_project_security_channel_id(const char *root,const char *organization,
    const char *project,char *channel_id,size_t capacity);

#endif
