#ifndef DIGIT_INTERFACE_H
#define DIGIT_INTERFACE_H

#include "module.h"

#define DIGIT_INTERFACE_DEFAULT_HOST "127.0.0.1"
#define DIGIT_INTERFACE_DEFAULT_PORT 8081

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
