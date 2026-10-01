#ifndef DIGIT_LESSON_H
#define DIGIT_LESSON_H

#include <stddef.h>
#include "module.h"

#define DIGIT_LESSON_INGEST_SERVICE "lesson.ingest"
#define DIGIT_LESSON_NAME_MAX 128
#define DIGIT_LESSON_PATH_MAX 512

typedef struct
{
    char name[DIGIT_LESSON_NAME_MAX];
} digit_lesson_ingest_request_t;

typedef struct
{
    int completed;
    size_t units;
    size_t stored;
    size_t rejected;
    char path[DIGIT_LESSON_PATH_MAX];
} digit_lesson_ingest_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
