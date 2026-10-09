#ifndef DIGIT_DISPATCHER_H
#define DIGIT_DISPATCHER_H

#include "module.h"

#define DIGIT_DISPATCHER_SERVICE "dispatcher.handle"
#define DIGIT_DISPATCHER_SCOPED_SERVICE "dispatcher.handle_scoped"
#define DIGIT_DISPATCHER_ACTOR_MAX 64
#define DIGIT_DISPATCHER_SCOPE_MAX 64
#define DIGIT_DISPATCHER_SCOPE_PRIVATE 1U
#define DIGIT_DISPATCHER_SCOPE_CHANNEL 2U
#define DIGIT_DISPATCHER_REQUEST_MAX 4096
#define DIGIT_DISPATCHER_ANSWER_MAX 4096

typedef struct
{
    char request[DIGIT_DISPATCHER_REQUEST_MAX];
} digit_dispatcher_request_t;

typedef struct
{
    int answered;
    char answer[DIGIT_DISPATCHER_ANSWER_MAX];
} digit_dispatcher_result_t;

/* [AI:GPT-6 | 2026-10-09] Additive, request-local context contract.
 * Identity and scope must originate in an authenticated Interface.
 * Presence of a value in this request is not proof of authentication.
 * No retained history or access is implied by this structure. */
typedef struct
{
    char request[DIGIT_DISPATCHER_REQUEST_MAX];
    char actor[DIGIT_DISPATCHER_ACTOR_MAX];
    unsigned int scope_kind;
    char organization[DIGIT_DISPATCHER_SCOPE_MAX];
    char project[DIGIT_DISPATCHER_SCOPE_MAX];
    char channel[DIGIT_DISPATCHER_SCOPE_MAX];
} digit_dispatcher_scoped_request_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
