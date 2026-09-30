#ifndef DIGIT_LLAMA_H
#define DIGIT_LLAMA_H

#include <stddef.h>
#include "module.h"

#define LLAMA_DEFAULT_HOST "127.0.0.1"
#define LLAMA_DEFAULT_PORT 8080
#define DIGIT_LLAMA_CONTEXT_SERVICE "llama.context"
#define DIGIT_LLAMA_TEXT_MAX 4096
#define DIGIT_LLAMA_CONTEXT_MAX 4096

typedef struct
{
    char text[DIGIT_LLAMA_TEXT_MAX];
} digit_llama_context_request_t;

typedef struct
{
    int available;
    char context[DIGIT_LLAMA_CONTEXT_MAX];
} digit_llama_context_result_t;

int llama_endpoint_reachable(void);
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
