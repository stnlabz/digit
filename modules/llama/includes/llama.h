#ifndef DIGIT_LLAMA_H
#define DIGIT_LLAMA_H

#include "module.h"

#define LLAMA_DEFAULT_HOST "127.0.0.1"
#define LLAMA_DEFAULT_PORT 8080

int llama_endpoint_reachable(void);

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
