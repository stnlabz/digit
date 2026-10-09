#include <stdio.h>
#include "dashboard_response.h"
/* [AI:GPT-6 | 2026-10-08] Never publish an unchecked or unauthorized
 * snapshot. Reject impossible counters and truncated JSON. */
int digit_dashboard_response(int authorized,size_t channels,size_t alerts,
                             size_t unacknowledged,size_t channel_limit,
                             size_t alert_limit,char *output,size_t capacity)
{
    int n;
    if(output && capacity)output[0]=0;
    if(authorized!=1 || !output || !capacity ||
       channels>channel_limit || alerts>alert_limit ||
       unacknowledged>alerts)return 0;
    n=snprintf(output,capacity,
      "{\"authorized\":true,\"scope\":\"digit-operations-read\","
      "\"channels\":%zu,\"alerts\":%zu,\"unacknowledged_alerts\":%zu}\n",
      channels,alerts,unacknowledged);
    if(n<=0 || (size_t)n>=capacity){output[0]=0;return 0;}
    return 1;
}
