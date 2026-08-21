/*
 * STN-LABZ
 * Digit Core
 *
 * digit_module_registry.c
 *
 * Core-controlled module lifecycle registry.
 */

#include <string.h>

#include "digit_module_registry.h"


static int digit_module_text_valid(
    const char *text,
    size_t capacity
)
{
    size_t length;


    if (
        text == NULL ||
        capacity == 0
    )
    {
        return 0;
    }


    length =
        strlen(
            text
        );


    if (
        length == 0 ||
        length >= capacity
    )
    {
        return 0;
    }


    return 1;
}


static digit_module_record_t *
digit_module_registry_find_mutable(
    digit_module_registry_t *registry,
    const char *module_id
)
{
    size_t index;


    if (
        registry == NULL ||
        module_id == NULL
    )
    {
        return NULL;
    }


    for (
        index = 0;
        index < registry->count;
        ++index
    )
    {
        if (
            strcmp(
                registry
                    ->modules[index]
                    .descriptor
                    .id,
                module_id
            ) == 0
        )
        {
            return
                &registry->modules[index];
        }
    }


    return NULL;
}


static digit_module_result_t
digit_module_audit(
    digit_module_registry_t *registry,
    const digit_module_record_t *module,
    digit_module_audit_event_t event,
    digit_module_state_t previous_state,
    digit_module_result_t result
)
{
    digit_module_audit_entry_t *entry;

    size_t length;


    if (
        registry == NULL ||
        module == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    if (
        registry->audit_count >=
        DIGIT_MODULE_AUDIT_MAX
    )
    {
        return
            DIGIT_MODULE_ERR_AUDIT_FULL;
    }


    entry =
        &registry->audit[
            registry->audit_count
        ];


    memset(
        entry,
        0,
        sizeof(*entry)
    );


    entry->sequence =
        registry->next_sequence++;


    entry->event =
        event;


    entry->previous_state =
        previous_state;


    entry->resulting_state =
        module->state;


    entry->result =
        result;


    length =
        strlen(
            module->descriptor.id
        );


    if (
        length >=
        sizeof(entry->module_id)
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_IDENTITY;
    }


    memcpy(
        entry->module_id,
        module->descriptor.id,
        length + 1
    );


    registry->audit_count++;


    return
        DIGIT_MODULE_OK;
}


void digit_module_registry_init(
    digit_module_registry_t *registry
)
{
    if (
        registry == NULL
    )
    {
        return;
    }


    memset(
        registry,
        0,
        sizeof(*registry)
    );


    registry->next_sequence =
        1;
}


digit_module_result_t
digit_module_registry_discover(
    digit_module_registry_t *registry,
    const digit_module_descriptor_t *descriptor
)
{
    digit_module_record_t *record;

    digit_module_result_t audit_result;


    if (
        registry == NULL ||
        descriptor == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    if (
        !digit_module_text_valid(
            descriptor->id,
            sizeof(descriptor->id)
        ) ||
        !digit_module_text_valid(
            descriptor->name,
            sizeof(descriptor->name)
        ) ||
        descriptor->qualify == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_IDENTITY;
    }


    if (
        digit_module_registry_find_mutable(
            registry,
            descriptor->id
        ) != NULL
    )
    {
        return
            DIGIT_MODULE_ERR_DUPLICATE;
    }


    if (
        registry->count >=
        DIGIT_MODULE_REGISTRY_MAX
    )
    {
        return
            DIGIT_MODULE_ERR_REGISTRY_FULL;
    }


    record =
        &registry->modules[
            registry->count
        ];


    memset(
        record,
        0,
        sizeof(*record)
    );


    record->descriptor =
        *descriptor;


    record->state =
        DIGIT_MODULE_STATE_DISCOVERED;


    record->activation_authorized =
        0;


    audit_result =
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_DISCOVERED,
            DIGIT_MODULE_STATE_DISCOVERED,
            DIGIT_MODULE_OK
        );


    if (
        audit_result !=
        DIGIT_MODULE_OK
    )
    {
        memset(
            record,
            0,
            sizeof(*record)
        );


        return
            audit_result;
    }


    registry->count++;


    return
        DIGIT_MODULE_OK;
}


digit_module_result_t
digit_module_registry_verify(
    digit_module_registry_t *registry,
    const char *module_id
)
{
    digit_module_record_t *record;

    digit_module_state_t previous;

    digit_module_result_t audit_result;


    record =
        digit_module_registry_find_mutable(
            registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    if (
        record->state ==
        DIGIT_MODULE_STATE_QUARANTINED
    )
    {
        return
            DIGIT_MODULE_ERR_QUARANTINED;
    }


    if (
        record->state !=
        DIGIT_MODULE_STATE_DISCOVERED
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_STATE;
    }


    previous =
        record->state;


    if (
        record
            ->descriptor
            .required_core_api_major !=
            DIGIT_MODULE_API_MAJOR ||
        record
            ->descriptor
            .required_core_api_minor >
            DIGIT_MODULE_API_MINOR
    )
    {
        record->state =
            DIGIT_MODULE_STATE_FAILED;


        (void)digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_FAILED,
            previous,
            DIGIT_MODULE_ERR_INCOMPATIBLE
        );


        return
            DIGIT_MODULE_ERR_INCOMPATIBLE;
    }


    record->state =
        DIGIT_MODULE_STATE_UNVERIFIED;


    audit_result =
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_VERIFIED,
            previous,
            DIGIT_MODULE_OK
        );


    if (
        audit_result !=
        DIGIT_MODULE_OK
    )
    {
        record->state =
            previous;


        return
            audit_result;
    }


    return
        DIGIT_MODULE_OK;
}


digit_module_result_t
digit_module_registry_qualify(
    digit_module_registry_t *registry,
    const char *module_id
)
{
    digit_module_record_t *record;

    digit_module_state_t previous;

    digit_module_result_t module_result;

    digit_module_result_t audit_result;

    digit_module_qualification_result_t report;


    record =
        digit_module_registry_find_mutable(
            registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    if (
        record->state ==
        DIGIT_MODULE_STATE_QUARANTINED
    )
    {
        return
            DIGIT_MODULE_ERR_QUARANTINED;
    }


    if (
        record->state !=
        DIGIT_MODULE_STATE_UNVERIFIED
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_STATE;
    }


    previous =
        record->state;


    record->state =
        DIGIT_MODULE_STATE_TESTING;


    audit_result =
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_TESTING,
            previous,
            DIGIT_MODULE_OK
        );


    if (
        audit_result !=
        DIGIT_MODULE_OK
    )
    {
        record->state =
            previous;


        return
            audit_result;
    }


    memset(
        &report,
        0,
        sizeof(report)
    );


    module_result =
        record
            ->descriptor
            .qualify(
                &report
            );


    record->qualification =
        report;


    previous =
        record->state;


    if (
        module_result !=
            DIGIT_MODULE_OK ||
        report.tests_executed <
            DIGIT_MODULE_MIN_TESTS ||
        report.tests_passed !=
            report.tests_executed ||
        report.tests_failed != 0 ||
        report.tests_passed +
            report.tests_failed !=
            report.tests_executed ||
        !report.negative_test_executed ||
        !report.negative_test_passed
    )
    {
        record->state =
            DIGIT_MODULE_STATE_FAILED;


        record->activation_authorized =
            0;


        (void)digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_FAILED,
            previous,
            DIGIT_MODULE_ERR_QUALIFICATION
        );


        return
            DIGIT_MODULE_ERR_QUALIFICATION;
    }


    record->state =
        DIGIT_MODULE_STATE_QUALIFIED;


    audit_result =
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_QUALIFIED,
            previous,
            DIGIT_MODULE_OK
        );


    if (
        audit_result !=
        DIGIT_MODULE_OK
    )
    {
        record->state =
            DIGIT_MODULE_STATE_FAILED;


        return
            audit_result;
    }


    return
        DIGIT_MODULE_OK;
}


digit_module_result_t
digit_module_registry_restore_qualification(
    digit_module_registry_t *registry,
    const char *module_id,
    const digit_module_qualification_result_t *qualification
)
{
    digit_module_record_t *record;

    digit_module_state_t previous;

    digit_module_result_t audit_result;


    if (
        registry == NULL ||
        module_id == NULL ||
        qualification == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_ARGUMENT;
    }


    record =
        digit_module_registry_find_mutable(
            registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    if (
        record->state ==
        DIGIT_MODULE_STATE_QUARANTINED
    )
    {
        return
            DIGIT_MODULE_ERR_QUARANTINED;
    }


    if (
        record->state !=
        DIGIT_MODULE_STATE_UNVERIFIED
    )
    {
        return
            DIGIT_MODULE_ERR_INVALID_STATE;
    }


    if (
        qualification->tests_executed <
            DIGIT_MODULE_MIN_TESTS ||
        qualification->tests_passed !=
            qualification->tests_executed ||
        qualification->tests_failed != 0 ||
        qualification->tests_passed +
            qualification->tests_failed !=
            qualification->tests_executed ||
        !qualification->negative_test_executed ||
        !qualification->negative_test_passed
    )
    {
        return
            DIGIT_MODULE_ERR_QUALIFICATION;
    }


    previous =
        record->state;


    record->qualification =
        *qualification;


    record->activation_authorized =
        0;


    record->state =
        DIGIT_MODULE_STATE_QUALIFIED;


    audit_result =
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_QUALIFIED,
            previous,
            DIGIT_MODULE_OK
        );


    if (
        audit_result !=
        DIGIT_MODULE_OK
    )
    {
        record->state =
            previous;


        memset(
            &record->qualification,
            0,
            sizeof(record->qualification)
        );


        return
            audit_result;
    }


    return
        DIGIT_MODULE_OK;
}


digit_module_result_t
digit_module_registry_authorize_activation(
    digit_module_registry_t *registry,
    const char *module_id
)
{
    digit_module_record_t *record;

    digit_module_result_t audit_result;


    record =
        digit_module_registry_find_mutable(
            registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    if (
        record->state ==
        DIGIT_MODULE_STATE_QUARANTINED
    )
    {
        return
            DIGIT_MODULE_ERR_QUARANTINED;
    }


    if (
        record->state !=
        DIGIT_MODULE_STATE_QUALIFIED
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_QUALIFIED;
    }


    record->activation_authorized =
        1;


    audit_result =
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_AUTHORIZED,
            record->state,
            DIGIT_MODULE_OK
        );


    if (
        audit_result !=
        DIGIT_MODULE_OK
    )
    {
        record->activation_authorized =
            0;


        return
            audit_result;
    }


    return
        DIGIT_MODULE_OK;
}


digit_module_result_t
digit_module_registry_activate(
    digit_module_registry_t *registry,
    const char *module_id
)
{
    digit_module_record_t *record;

    digit_module_state_t previous;

    digit_module_result_t audit_result;


    record =
        digit_module_registry_find_mutable(
            registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    if (
        record->state ==
        DIGIT_MODULE_STATE_QUARANTINED
    )
    {
        return
            DIGIT_MODULE_ERR_QUARANTINED;
    }


    if (
        record->state !=
        DIGIT_MODULE_STATE_QUALIFIED
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_QUALIFIED;
    }


    if (
        !record->activation_authorized
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_AUTHORIZED;
    }


    previous =
        record->state;


    record->state =
        DIGIT_MODULE_STATE_ACTIVE;


    audit_result =
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_ACTIVE,
            previous,
            DIGIT_MODULE_OK
        );


    if (
        audit_result !=
        DIGIT_MODULE_OK
    )
    {
        record->state =
            previous;


        return
            audit_result;
    }


    return
        DIGIT_MODULE_OK;
}


digit_module_result_t
digit_module_registry_fail(
    digit_module_registry_t *registry,
    const char *module_id
)
{
    digit_module_record_t *record;

    digit_module_state_t previous;


    record =
        digit_module_registry_find_mutable(
            registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    previous =
        record->state;


    record->state =
        DIGIT_MODULE_STATE_FAILED;


    record->activation_authorized =
        0;


    return
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_FAILED,
            previous,
            DIGIT_MODULE_OK
        );
}


digit_module_result_t
digit_module_registry_quarantine(
    digit_module_registry_t *registry,
    const char *module_id
)
{
    digit_module_record_t *record;

    digit_module_state_t previous;


    record =
        digit_module_registry_find_mutable(
            registry,
            module_id
        );


    if (
        record == NULL
    )
    {
        return
            DIGIT_MODULE_ERR_NOT_FOUND;
    }


    previous =
        record->state;


    record->state =
        DIGIT_MODULE_STATE_QUARANTINED;


    record->activation_authorized =
        0;


    return
        digit_module_audit(
            registry,
            record,
            DIGIT_MODULE_AUDIT_QUARANTINED,
            previous,
            DIGIT_MODULE_OK
        );
}


const digit_module_record_t *
digit_module_registry_find(
    const digit_module_registry_t *registry,
    const char *module_id
)
{
    size_t index;


    if (
        registry == NULL ||
        module_id == NULL
    )
    {
        return NULL;
    }


    for (
        index = 0;
        index < registry->count;
        ++index
    )
    {
        if (
            strcmp(
                registry
                    ->modules[index]
                    .descriptor
                    .id,
                module_id
            ) == 0
        )
        {
            return
                &registry->modules[index];
        }
    }


    return NULL;
}