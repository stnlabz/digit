#ifndef DIGIT_MEMORY_H
#define DIGIT_MEMORY_H
#include "module.h"
#define DIGIT_MEMORY_PUT_SERVICE "memory.put"
#define DIGIT_MEMORY_GET_SERVICE "memory.get"
#define DIGIT_MEMORY_ACTOR_MAX 64
#define DIGIT_MEMORY_TEXT_MAX 4096
typedef struct { char actor[DIGIT_MEMORY_ACTOR_MAX]; char text[DIGIT_MEMORY_TEXT_MAX]; } digit_memory_record_t;
typedef struct { int ok; char text[DIGIT_MEMORY_TEXT_MAX]; } digit_memory_result_t;
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);
#endif
