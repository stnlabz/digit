#ifndef DIGIT_CONTROLLED_COMMUNICATION_H
#define DIGIT_CONTROLLED_COMMUNICATION_H
#include <stddef.h>
#include "channel.h"
/* [AI:GPT-6 | 2026-10-08] Controlled communication receipt.
 * PERSISTED is only an acknowledgement from the established Core append
 * service, not delivery to a human or an external client. */
int digit_controlled_message_valid(const char *channel,const char *origin,
                                   const char *body);
int digit_controlled_receipt(const digit_channel_message_t *message,
                             const char *channel,const char *origin,
                             char *output,size_t capacity);
#endif
