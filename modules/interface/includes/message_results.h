#ifndef DIGIT_INTERFACE_MESSAGE_RESULTS_H
#define DIGIT_INTERFACE_MESSAGE_RESULTS_H
#include <stddef.h>
#include "channel.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.7: validate channel message response bounds. */
int digit_interface_messages_valid(const digit_channel_message_t *messages,
 size_t count,size_t capacity,const char *channel_id);
#endif
