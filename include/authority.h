#ifndef DIGIT_AUTHORITY_H
#define DIGIT_AUTHORITY_H

#include "module_registry.h"
#include "qualification.h"

typedef enum
{
    DIGIT_AUTHORITY_DENY = 0,
    DIGIT_AUTHORITY_ALLOW = 1
} digit_authority_decision_t;

int digit_authority_requires_qualification(
    const digit_qualification_inventory_t *inventory,
    const stnlabz_module_descriptor_t *descriptor
);

digit_authority_decision_t digit_authority_may_activate(
    const stnlabz_module_record_t *record
);

digit_authority_decision_t digit_authority_may_replace(
    const stnlabz_module_record_t *active,
    const stnlabz_module_record_t *candidate
);

digit_authority_decision_t digit_authority_may_dispatch(
    const stnlabz_module_record_t *record
);

#endif
