#ifndef DIGIT_ALERTS_CHANNEL_H
#define DIGIT_ALERTS_CHANNEL_H
#include <stddef.h>
#include "module.h"
/* [AI:GPT-6 | 2026-10-09] Project-bound permanent Alerts channel.
 * Only an assigned SA/project member may initiate or retrieve binding. */
int digit_alerts_channel_ensure(const char *root,const char *org,const char *project,
 const char *actor,const char *sa_registry,const stnlabz_module_host_t *host,
 char *channel,size_t capacity);
int digit_alerts_channel_lookup(const char *root,const char *org,const char *project,
 char *channel,size_t capacity);
#endif
