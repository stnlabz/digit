#ifndef DIGIT_INTERFACE_CHANNEL_RESULTS_H
#define DIGIT_INTERFACE_CHANNEL_RESULTS_H
#include <stddef.h>
#include "channel.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.10: guard Core channel list
 * and exact-record identity at the Interface boundary. */
int digit_interface_channels_valid(const digit_channel_t *channels,
                                   size_t count,size_t capacity);
int digit_interface_channel_exact_valid(const digit_channel_t *channel,
                                        int found,const char *requested_id);
#endif
