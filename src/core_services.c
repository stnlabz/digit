#include <string.h>

#include "core_services.h"
#include "core_sa.h"
#include "module.h"
#include "service_registry.h"

static stnlabz_module_result_t channel_create_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_channel_create_request_t *in=request;digit_channel_create_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->created=digit_channel_create(in->name,&out->channel);*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t channel_list_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){digit_channel_list_response_t *out=response;(void)request;(void)context;if(request_size!=0||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->count=digit_channel_list(out->channels,DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX);if(out->count>DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX)out->count=DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX;*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t channel_get_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_channel_get_request_t *in=request;digit_channel_get_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->found=digit_channel_get(in->channel_id,&out->channel);*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t channel_message_append_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_channel_message_append_request_t *in=request;digit_channel_message_append_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->appended=digit_channel_message_append(in->channel_id,in->origin,in->body,&out->message);*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t channel_message_list_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_channel_message_list_request_t *in=request;digit_channel_message_list_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->count=digit_channel_message_list(in->channel_id,out->messages,DIGIT_CORE_SERVICE_MESSAGE_LIST_MAX);if(out->count>DIGIT_CORE_SERVICE_MESSAGE_LIST_MAX)out->count=DIGIT_CORE_SERVICE_MESSAGE_LIST_MAX;*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t alert_raise_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_alert_raise_request_t *in=request;digit_alert_raise_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->raised=digit_alert_raise(in->severity,in->source,in->summary,in->detail,in->operational_state,&out->alert);*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t alert_list_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_alert_list_request_t *in=request;digit_alert_list_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->count=digit_alert_list(out->alerts,DIGIT_CORE_SERVICE_ALERT_LIST_MAX,in->unacknowledged_only);if(out->count>DIGIT_CORE_SERVICE_ALERT_LIST_MAX)out->count=DIGIT_CORE_SERVICE_ALERT_LIST_MAX;*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t alert_get_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_alert_get_request_t *in=request;digit_alert_get_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->found=digit_alert_get(in->alert_id,&out->alert);*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t alert_ack_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context){const digit_alert_acknowledge_request_t *in=request;digit_alert_acknowledge_response_t *out=response;(void)context;if(!in||request_size!=sizeof(*in)||!out||response_size<sizeof(*out)||!response_used)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(out,0,sizeof(*out));out->acknowledged=digit_alert_acknowledge(in->alert_id,&out->alert);*response_used=sizeof(*out);return STNLABZ_MODULE_OK;}

/* [AI:GPT-6 | 2026-10-08] Core-owned SA gate; the caller supplies an
 * authenticated identity, not a role or authorization decision. */
static stnlabz_module_result_t digit_admin_sa_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *context)
{
    const digit_admin_sa_request_t *in=request;
    digit_admin_sa_response_t *out=response;
    (void)context;
    if(!in || request_size!=sizeof(*in) || !out || response_size<sizeof(*out) || !response_used)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(memchr(in->identity,0,sizeof(in->identity))!=NULL)
        out->authorized=digit_core_sa_verify(DIGIT_ADMIN_SA_REGISTRY,DIGIT_ADMIN_ORGANIZATION,in->identity);
    *response_used=sizeof(*out);
    return STNLABZ_MODULE_OK;
}

int digit_core_services_register(void)
{
    if(!digit_service_register(DIGIT_CHANNEL_SERVICE_CREATE,channel_create_service,NULL))return 0;
    if(!digit_service_register(DIGIT_CHANNEL_SERVICE_LIST,channel_list_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_CHANNEL_SERVICE_GET,channel_get_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_CHANNEL_SERVICE_MESSAGE_APPEND,channel_message_append_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_CHANNEL_SERVICE_MESSAGE_LIST,channel_message_list_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_ALERT_SERVICE_RAISE,alert_raise_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_ALERT_SERVICE_LIST,alert_list_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_ALERT_SERVICE_GET,alert_get_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_ALERT_SERVICE_ACKNOWLEDGE,alert_ack_service,NULL))goto fail;
    if(!digit_service_register(DIGIT_ADMIN_SA_SERVICE,digit_admin_sa_service,NULL))goto fail;
    return 1;
fail:
    digit_core_services_unregister();return 0;
}

void digit_core_services_unregister(void)
{
    (void)digit_service_unregister(DIGIT_ADMIN_SA_SERVICE,NULL);(void)digit_service_unregister(DIGIT_ALERT_SERVICE_ACKNOWLEDGE,NULL);(void)digit_service_unregister(DIGIT_ALERT_SERVICE_GET,NULL);(void)digit_service_unregister(DIGIT_ALERT_SERVICE_LIST,NULL);(void)digit_service_unregister(DIGIT_ALERT_SERVICE_RAISE,NULL);(void)digit_service_unregister(DIGIT_CHANNEL_SERVICE_MESSAGE_LIST,NULL);(void)digit_service_unregister(DIGIT_CHANNEL_SERVICE_MESSAGE_APPEND,NULL);(void)digit_service_unregister(DIGIT_CHANNEL_SERVICE_GET,NULL);(void)digit_service_unregister(DIGIT_CHANNEL_SERVICE_LIST,NULL);(void)digit_service_unregister(DIGIT_CHANNEL_SERVICE_CREATE,NULL);
}
