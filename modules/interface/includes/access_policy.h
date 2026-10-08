#ifndef DIGIT_INTERFACE_ACCESS_POLICY_H
#define DIGIT_INTERFACE_ACCESS_POLICY_H

#include <stddef.h>
#include <string.h>

/* [AI:GPT-6 | 2026-10-08] Deterministic, deny-by-default resource access predicate.
 * This is a pure eligibility/ACL component, NOT an authentication mechanism.
 * A caller must supply facts from authoritative authenticated records.
 * No role (including admin) bypasses any mandatory qualification condition.
 */

typedef struct {
    const char *user_id;
    const char *organization_id;
    int account_active;
    int authenticated;
    int qualification_valid;
    int job_assignment_active;
    int mission_qualification_valid;
} digit_access_principal_t;

typedef struct {
    const char *organization_id;
    const char *project_id;
    const char *channel_id;
    const char *authorized_user_id;
    int acl_grant_active;
} digit_access_resource_t;

typedef enum {
    DIGIT_ACCESS_DENIED = 0,
    DIGIT_ACCESS_ELIGIBLE = 1
} digit_access_result_t;

static inline int digit_access_nonempty(const char *value)
{
    return value != NULL && value[0] != '\0';
}

/* Exact identity and scope matches are required. NULL or incomplete records
 * deny access; this function never grants access based on a claimed role.
 * The caller must check that records and grant are authoritative and current.
 */
static inline digit_access_result_t digit_access_evaluate(
    const digit_access_principal_t *principal,
    const digit_access_resource_t *resource)
{
    if (principal == NULL || resource == NULL) return DIGIT_ACCESS_DENIED;
    if (!digit_access_nonempty(principal->user_id) ||
        !digit_access_nonempty(principal->organization_id) ||
        !digit_access_nonempty(resource->organization_id) ||
        !digit_access_nonempty(resource->project_id) ||
        !digit_access_nonempty(resource->channel_id) ||
        !digit_access_nonempty(resource->authorized_user_id))
        return DIGIT_ACCESS_DENIED;

    if (principal->authenticated != 1 || principal->account_active != 1 ||
        principal->qualification_valid != 1 ||
        principal->job_assignment_active != 1 ||
        principal->mission_qualification_valid != 1 ||
        resource->acl_grant_active != 1)
        return DIGIT_ACCESS_DENIED;

    if (strcmp(principal->user_id, resource->authorized_user_id) != 0 ||
        strcmp(principal->organization_id, resource->organization_id) != 0)
        return DIGIT_ACCESS_DENIED;

    return DIGIT_ACCESS_ELIGIBLE;
}

#endif
