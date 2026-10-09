#ifndef DIGIT_INTERFACE_DASHBOARD_RESPONSE_H
#define DIGIT_INTERFACE_DASHBOARD_RESPONSE_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-08] Interface 1.5.6 bounded SA snapshot rendering.
 * Authorization is enforced by Core before this function is called. */
int digit_dashboard_response(int authorized,size_t channels,size_t alerts,
                             size_t unacknowledged,size_t channel_limit,
                             size_t alert_limit,char *output,size_t capacity);
#endif
