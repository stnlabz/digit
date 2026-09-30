#ifndef DIGIT_CORE_SERVICES_H
#define DIGIT_CORE_SERVICES_H

#include <stddef.h>

#include "alert.h"
#include "channel.h"

#define DIGIT_CHANNEL_SERVICE_CREATE "channel.create"
#define DIGIT_CHANNEL_SERVICE_LIST "channel.list"
#define DIGIT_CHANNEL_SERVICE_GET "channel.get"
#define DIGIT_CHANNEL_SERVICE_MESSAGE_APPEND "channel.message.append"
#define DIGIT_CHANNEL_SERVICE_MESSAGE_LIST "channel.message.list"
#define DIGIT_ALERT_SERVICE_RAISE "alert.raise"
#define DIGIT_ALERT_SERVICE_LIST "alert.list"
#define DIGIT_ALERT_SERVICE_GET "alert.get"
#define DIGIT_ALERT_SERVICE_ACKNOWLEDGE "alert.acknowledge"

#define DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX 128
#define DIGIT_CORE_SERVICE_MESSAGE_LIST_MAX 256
#define DIGIT_CORE_SERVICE_ALERT_LIST_MAX 256

typedef struct { char name[DIGIT_CHANNEL_NAME_MAX]; } digit_channel_create_request_t;
typedef struct { int created; digit_channel_t channel; } digit_channel_create_response_t;
typedef struct { size_t count; digit_channel_t channels[DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX]; } digit_channel_list_response_t;
typedef struct { char channel_id[DIGIT_CHANNEL_ID_MAX]; } digit_channel_get_request_t;
typedef struct { int found; digit_channel_t channel; } digit_channel_get_response_t;
typedef struct { char channel_id[DIGIT_CHANNEL_ID_MAX]; char origin[DIGIT_CHANNEL_ORIGIN_MAX]; char body[DIGIT_CHANNEL_MESSAGE_MAX]; } digit_channel_message_append_request_t;
typedef struct { int appended; digit_channel_message_t message; } digit_channel_message_append_response_t;
typedef struct { char channel_id[DIGIT_CHANNEL_ID_MAX]; } digit_channel_message_list_request_t;
typedef struct { size_t count; digit_channel_message_t messages[DIGIT_CORE_SERVICE_MESSAGE_LIST_MAX]; } digit_channel_message_list_response_t;

typedef struct { digit_alert_severity_t severity; char source[DIGIT_ALERT_SOURCE_MAX]; char summary[DIGIT_ALERT_SUMMARY_MAX]; char detail[DIGIT_ALERT_DETAIL_MAX]; char operational_state[DIGIT_ALERT_STATE_MAX]; } digit_alert_raise_request_t;
typedef struct { int raised; digit_alert_t alert; } digit_alert_raise_response_t;
typedef struct { int unacknowledged_only; } digit_alert_list_request_t;
typedef struct { size_t count; digit_alert_t alerts[DIGIT_CORE_SERVICE_ALERT_LIST_MAX]; } digit_alert_list_response_t;
typedef struct { char alert_id[DIGIT_ALERT_ID_MAX]; } digit_alert_get_request_t;
typedef struct { int found; digit_alert_t alert; } digit_alert_get_response_t;
typedef struct { char alert_id[DIGIT_ALERT_ID_MAX]; } digit_alert_acknowledge_request_t;
typedef struct { int acknowledged; digit_alert_t alert; } digit_alert_acknowledge_response_t;

int digit_core_services_register(void);
void digit_core_services_unregister(void);

#endif
