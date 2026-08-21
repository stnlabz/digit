/*
 * STN-LABZ
 * Digit Core
 *
 * digit_module.c
 *
 * Common module lifecycle and result strings.
 */

#include "digit_module.h"


const char *
digit_module_state_string(
    digit_module_state_t state
)
{
    switch (
        state
    )
    {
        case DIGIT_MODULE_STATE_DISCOVERED:

            return "DISCOVERED";


        case DIGIT_MODULE_STATE_UNVERIFIED:

            return "UNVERIFIED";


        case DIGIT_MODULE_STATE_TESTING:

            return "TESTING";


        case DIGIT_MODULE_STATE_QUALIFIED:

            return "QUALIFIED";


        case DIGIT_MODULE_STATE_ACTIVE:

            return "ACTIVE";


        case DIGIT_MODULE_STATE_FAILED:

            return "FAILED";


        case DIGIT_MODULE_STATE_QUARANTINED:

            return "QUARANTINED";


        default:

            return "UNKNOWN";
    }
}


const char *
digit_module_result_string(
    digit_module_result_t result
)
{
    switch (
        result
    )
    {
        case DIGIT_MODULE_OK:

            return "OK";


        case DIGIT_MODULE_ERR_INVALID_ARGUMENT:

            return "INVALID_ARGUMENT";


        case DIGIT_MODULE_ERR_INVALID_IDENTITY:

            return "INVALID_IDENTITY";


        case DIGIT_MODULE_ERR_DUPLICATE:

            return "DUPLICATE";


        case DIGIT_MODULE_ERR_REGISTRY_FULL:

            return "REGISTRY_FULL";


        case DIGIT_MODULE_ERR_NOT_FOUND:

            return "NOT_FOUND";


        case DIGIT_MODULE_ERR_INCOMPATIBLE:

            return "INCOMPATIBLE";


        case DIGIT_MODULE_ERR_INVALID_STATE:

            return "INVALID_STATE";


        case DIGIT_MODULE_ERR_QUALIFICATION:

            return "QUALIFICATION_FAILED";


        case DIGIT_MODULE_ERR_NOT_QUALIFIED:

            return "NOT_QUALIFIED";


        case DIGIT_MODULE_ERR_NOT_AUTHORIZED:

            return "NOT_AUTHORIZED";


        case DIGIT_MODULE_ERR_QUARANTINED:

            return "QUARANTINED";


        case DIGIT_MODULE_ERR_AUDIT_FULL:

            return "AUDIT_FULL";


        case DIGIT_MODULE_ERR_START_FAILED:

            return "START_FAILED";


        case DIGIT_MODULE_ERR_STOP_FAILED:

            return "STOP_FAILED";


        default:

            return "UNKNOWN";
    }
}