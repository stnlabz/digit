#ifndef DIGIT_STN2_H
#define DIGIT_STN2_H
#include "module.h"
#define DIGIT_STN2_SERVICE "stn2.generate_intel"
#define DIGIT_STN2_TEXT_MAX 4096
typedef struct {char command[32];char actor[64];} digit_stn2_request_t;
typedef struct {unsigned int sources_ok,sources_failed;char report[DIGIT_STN2_TEXT_MAX];} digit_stn2_result_t;
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);
#endif
