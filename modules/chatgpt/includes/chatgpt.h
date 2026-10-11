#ifndef DIGIT_CHATGPT_H
#define DIGIT_CHATGPT_H
#include <stddef.h>
#include "module.h"
/* [AI:GPT-6 | 2026-10-11] ChatGPT integration contract. No authority from conversation text. */
#define DIGIT_CHATGPT_CAPABILITIES_SERVICE "chatgpt.capabilities"
#define DIGIT_CHATGPT_TEST_SERVICE "chatgpt.test_case"
#define DIGIT_CHATGPT_REQUEST_MAX 2048
typedef struct {unsigned int version, interface_bound, non_mutating_qualified, conversations_enabled;} digit_chatgpt_capabilities_t;
typedef struct {char request[DIGIT_CHATGPT_REQUEST_MAX];} digit_chatgpt_test_request_t;
typedef struct {unsigned int accepted, dispatched;} digit_chatgpt_test_result_t;
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);
#endif
