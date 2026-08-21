#ifndef DIGIT_MODULE_REGISTRY_H
#define DIGIT_MODULE_REGISTRY_H

#include <stddef.h>

#include "digit_module.h"


#define DIGIT_MODULE_REGISTRY_MAX \
    32

#define DIGIT_MODULE_AUDIT_MAX \
    128


/*
 * ------------------------------------------------
 * MODULE AUDIT EVENTS
 * ------------------------------------------------
 */

typedef enum
{
    DIGIT_MODULE_AUDIT_DISCOVERED = 0,

    DIGIT_MODULE_AUDIT_VERIFIED,

    DIGIT_MODULE_AUDIT_TESTING,

    DIGIT_MODULE_AUDIT_QUALIFIED,

    DIGIT_MODULE_AUDIT_FAILED,

    DIGIT_MODULE_AUDIT_AUTHORIZED,

    DIGIT_MODULE_AUDIT_ACTIVE,

    DIGIT_MODULE_AUDIT_QUARANTINED

} digit_module_audit_event_t;


/*
 * ------------------------------------------------
 * MODULE RECORD
 * ------------------------------------------------
 */

typedef struct
{
    digit_module_descriptor_t descriptor;

    digit_module_state_t state;

    digit_module_qualification_result_t qualification;

    int activation_authorized;

} digit_module_record_t;


/*
 * ------------------------------------------------
 * MODULE AUDIT ENTRY
 * ------------------------------------------------
 */

typedef struct
{
    unsigned long sequence;

    char module_id[
        DIGIT_MODULE_ID_MAX
    ];

    digit_module_audit_event_t event;

    digit_module_state_t previous_state;

    digit_module_state_t resulting_state;

    digit_module_result_t result;

} digit_module_audit_entry_t;


/*
 * ------------------------------------------------
 * MODULE REGISTRY
 * ------------------------------------------------
 */

typedef struct
{
    digit_module_record_t modules[
        DIGIT_MODULE_REGISTRY_MAX
    ];

    size_t count;

    digit_module_audit_entry_t audit[
        DIGIT_MODULE_AUDIT_MAX
    ];

    size_t audit_count;

    unsigned long next_sequence;

} digit_module_registry_t;


/*
 * ------------------------------------------------
 * REGISTRY API
 * ------------------------------------------------
 */

void digit_module_registry_init(
    digit_module_registry_t* registry
);


digit_module_result_t
digit_module_registry_discover(
    digit_module_registry_t* registry,
    const digit_module_descriptor_t* descriptor
);


digit_module_result_t
digit_module_registry_verify(
    digit_module_registry_t* registry,
    const char* module_id
);


digit_module_result_t
digit_module_registry_qualify(
    digit_module_registry_t* registry,
    const char* module_id
);


digit_module_result_t
digit_module_registry_restore_qualification(
    digit_module_registry_t* registry,
    const char* module_id,
    const digit_module_qualification_result_t* qualification
);


digit_module_result_t
digit_module_registry_authorize_activation(
    digit_module_registry_t* registry,
    const char* module_id
);


digit_module_result_t
digit_module_registry_activate(
    digit_module_registry_t* registry,
    const char* module_id
);


digit_module_result_t
digit_module_registry_fail(
    digit_module_registry_t* registry,
    const char* module_id
);


digit_module_result_t
digit_module_registry_quarantine(
    digit_module_registry_t* registry,
    const char* module_id
);


const digit_module_record_t*
digit_module_registry_find(
    const digit_module_registry_t* registry,
    const char* module_id
);


#endif