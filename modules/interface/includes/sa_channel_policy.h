#ifndef DIGIT_SA_CHANNEL_POLICY_H
#define DIGIT_SA_CHANNEL_POLICY_H
/* [AI:GPT-6 | 2026-10-09] Organization-scoped permanent
 * administrative channel names. No role implies cross-org grants. */
int digit_sa_channel_permanent(const char *channel_name);
int digit_sa_channel_allowed(const char *requested_org,const char *assigned_org,
                             int qualified,int assigned,int mission_qualified,
                             const char *channel_name);
#endif
