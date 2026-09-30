#include "authority.h"

int digit_authority_requires_qualification(
    const digit_qualification_inventory_t *inventory,
    const stnlabz_module_descriptor_t *descriptor
)
{
    if (inventory == NULL || descriptor == NULL)
    {
        return 1;
    }

    return !digit_qualification_contains(inventory, descriptor);
}

digit_authority_decision_t digit_authority_may_activate(
    const stnlabz_module_record_t *record
)
{
    if (record == NULL)
    {
        return DIGIT_AUTHORITY_DENY;
    }

    if (record->state != STNLABZ_MODULE_STATE_QUALIFIED &&
        record->state != STNLABZ_MODULE_STATE_STOPPED)
    {
        return DIGIT_AUTHORITY_DENY;
    }

    if (record->qualification.tests_executed < STNLABZ_MODULE_MIN_TESTS ||
        record->qualification.tests_failed != 0 ||
        !record->qualification.negative_test_executed ||
        !record->qualification.negative_test_passed ||
        !record->activation_authorized)
    {
        return DIGIT_AUTHORITY_DENY;
    }

    return DIGIT_AUTHORITY_ALLOW;
}

digit_authority_decision_t digit_authority_may_replace(
    const stnlabz_module_record_t *active,
    const stnlabz_module_record_t *candidate
)
{
    if (active == NULL || candidate == NULL)
    {
        return DIGIT_AUTHORITY_DENY;
    }

    if (active->state != STNLABZ_MODULE_STATE_ACTIVE)
    {
        return DIGIT_AUTHORITY_DENY;
    }

    if (digit_authority_may_activate(candidate) != DIGIT_AUTHORITY_ALLOW)
    {
        return DIGIT_AUTHORITY_DENY;
    }

    return DIGIT_AUTHORITY_ALLOW;
}

digit_authority_decision_t digit_authority_may_dispatch(
    const stnlabz_module_record_t *record
)
{
    if (record == NULL)
    {
        return DIGIT_AUTHORITY_DENY;
    }

    return record->state == STNLABZ_MODULE_STATE_ACTIVE
        ? DIGIT_AUTHORITY_ALLOW
        : DIGIT_AUTHORITY_DENY;
}
