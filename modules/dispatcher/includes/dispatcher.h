#ifndef DIGIT_DISPATCHER_H
#define DIGIT_DISPATCHER_H

#include "module.h"

#define DIGIT_DISPATCHER_SERVICE "dispatcher.handle"
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

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
