#ifndef DIGIT_INTERFACE_CHANNEL_CREATE_POLICY_H
#define DIGIT_INTERFACE_CHANNEL_CREATE_POLICY_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-09] 1.5.8: independent input validation.
 * Authorization must be independently established by Core. */
int digit_interface_channel_create_name(const char *name,size_t capacity);
#endif
