#ifndef DIGIT_INTERFACE_CHANNEL_ACL_H
#define DIGIT_INTERFACE_CHANNEL_ACL_H

#include <stddef.h>
#include "access_policy.h"

/* [AI:GPT-6 | 2026-10-08] Operator-controlled authorization records:
 * grants.tsv:
 * user<TAB>organization<TAB>project<TAB>channel<TAB>qualified(0|1)
 * <TAB>assigned(0|1)<TAB>mission_qualified(0|1)<TAB>grant(0|1)\n
 * No implicit memberships, superuser bypass, or user-facing grant API.
 * Channel is the immutable channel ID from the Core channel registry.
 */
#define DIGIT_CHANNEL_ACL_PATH "/opt/digit/state/auth/channel_grants.tsv"

/* Restricted Core security channels require current SA qualification in addition
 * to the ordinary user/project/channel grant. The _scoped variant permits
 * isolated test fixtures; the wrapper uses approved production paths. */
int digit_channel_acl_check_scoped(const char *path,const char *project_root,
                                  const char *sa_registry,
                                  const char *verified_user,const char *channel_id);
int digit_channel_acl_check_file(const char *path, const char *verified_user,
                                  const char *channel_id);
#endif
