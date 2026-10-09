#include <string.h>
#include "sa_channel_policy.h"
/* [AI:GPT-6 | 2026-10-09] Explicit, case-sensitive channel names.
 * Every organization independently owns its Security and Alerts channels. */
int digit_sa_channel_permanent(const char *channel_name)
{
    return channel_name &&
        (strcmp(channel_name,"Security")==0 || strcmp(channel_name,"Alerts")==0);
}
int digit_sa_channel_allowed(const char *requested_org,const char *assigned_org,
                             int qualified,int assigned,int mission_qualified,
                             const char *channel_name)
{
    return requested_org && requested_org[0] && assigned_org && assigned_org[0] &&
        strcmp(requested_org,assigned_org)==0 &&
        qualified==1 && assigned==1 && mission_qualified==1 &&
        digit_sa_channel_permanent(channel_name);
}
