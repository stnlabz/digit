#ifndef DIGIT_MODULE_H
#define DIGIT_MODULE_H


#define DIGIT_MODULE_ID_MAX \
    64

#define DIGIT_MODULE_NAME_MAX \
    64

#define DIGIT_MODULE_COMMAND_NAME_MAX \
    64

#define DIGIT_MODULE_COMMAND_ARGUMENTS_MAX \
    512

#define DIGIT_MODULE_COMMAND_SENDER_MAX \
    128

#define DIGIT_MODULE_COMMAND_ACCOUNT_MAX \
    128

#define DIGIT_MODULE_MIN_TESTS \
    10


/*
 * ------------------------------------------------
 * CORE MODULE API
 * ------------------------------------------------
 */

#define DIGIT_MODULE_API_MAJOR \
    1

#define DIGIT_MODULE_API_MINOR \
    3


 /*
  * ------------------------------------------------
  * MODULE LIFECYCLE
  * ------------------------------------------------
  */

typedef enum
{
    DIGIT_MODULE_STATE_DISCOVERED = 0,

    DIGIT_MODULE_STATE_UNVERIFIED,

    DIGIT_MODULE_STATE_TESTING,

    DIGIT_MODULE_STATE_QUALIFIED,

    DIGIT_MODULE_STATE_ACTIVE,

    DIGIT_MODULE_STATE_FAILED,

    DIGIT_MODULE_STATE_QUARANTINED

} digit_module_state_t;


/*
 * ------------------------------------------------
 * MODULE RESULTS
 * ------------------------------------------------
 */

typedef enum
{
    DIGIT_MODULE_OK = 0,

    DIGIT_MODULE_ERR_INVALID_ARGUMENT,

    DIGIT_MODULE_ERR_INVALID_IDENTITY,

    DIGIT_MODULE_ERR_DUPLICATE,

    DIGIT_MODULE_ERR_REGISTRY_FULL,

    DIGIT_MODULE_ERR_NOT_FOUND,

    DIGIT_MODULE_ERR_INCOMPATIBLE,

    DIGIT_MODULE_ERR_INVALID_STATE,

    DIGIT_MODULE_ERR_QUALIFICATION,

    DIGIT_MODULE_ERR_NOT_QUALIFIED,

    DIGIT_MODULE_ERR_NOT_AUTHORIZED,

    DIGIT_MODULE_ERR_QUARANTINED,

    DIGIT_MODULE_ERR_AUDIT_FULL,

    DIGIT_MODULE_ERR_START_FAILED,

    DIGIT_MODULE_ERR_STOP_FAILED

} digit_module_result_t;


/*
 * ------------------------------------------------
 * QUALIFICATION RESULT
 * ------------------------------------------------
 */

typedef struct
{
    unsigned int tests_executed;

    unsigned int tests_passed;

    unsigned int tests_failed;

    int negative_test_executed;

    int negative_test_passed;

} digit_module_qualification_result_t;


/*
 * ------------------------------------------------
 * MODULE COMMAND
 * ------------------------------------------------
 *
 * Generic command representation exposed through
 * the Core module ABI.
 *
 * Modules do not depend on Core command internals.
 */

typedef struct
{
    char sender[
        DIGIT_MODULE_COMMAND_SENDER_MAX
    ];

    char account[
        DIGIT_MODULE_COMMAND_ACCOUNT_MAX
    ];

    char name[
        DIGIT_MODULE_COMMAND_NAME_MAX
    ];

    char arguments[
        DIGIT_MODULE_COMMAND_ARGUMENTS_MAX
    ];

} digit_module_command_t;


/*
 * ------------------------------------------------
 * MODULE COMMAND REPLY
 * ------------------------------------------------
 */

typedef int
(*digit_module_command_reply_fn)(
    void* reply_context,
    const char* message
    );


/*
 * ------------------------------------------------
 * MODULE COMMAND HANDLER
 * ------------------------------------------------
 */

typedef digit_module_result_t
(*digit_module_command_handler_fn)(
    const digit_module_command_t* command,
    digit_module_command_reply_fn reply,
    void* reply_context,
    void* handler_context
    );


/*
 * ------------------------------------------------
 * CORE HOST SERVICES
 * ------------------------------------------------
 */

typedef int
(*digit_module_send_message_fn)(
    const char* message
    );


typedef int
(*digit_module_register_command_fn)(
    const char* name,
    digit_module_command_handler_fn handler,
    void* handler_context
    );


typedef int
(*digit_module_unregister_command_fn)(
    const char* name,
    void* handler_context
    );


/*
 * ------------------------------------------------
 * CORE HOST API
 * ------------------------------------------------
 *
 * Modules do not receive direct access to Core
 * internals.
 *
 * Core exposes only operations represented through
 * this ABI.
 */

typedef struct
{
    digit_module_send_message_fn
        send_message;

    digit_module_register_command_fn
        register_command;

    digit_module_unregister_command_fn
        unregister_command;

} digit_module_host_t;


/*
 * ------------------------------------------------
 * MODULE CALLBACKS
 * ------------------------------------------------
 */

typedef digit_module_result_t
(*digit_module_qualify_fn)(
    digit_module_qualification_result_t* result
    );


typedef digit_module_result_t
(*digit_module_start_fn)(
    const digit_module_host_t* host
    );


typedef digit_module_result_t
(*digit_module_stop_fn)(void);


/*
 * ------------------------------------------------
 * MODULE DESCRIPTOR
 * ------------------------------------------------
 */

typedef struct
{
    char id[
        DIGIT_MODULE_ID_MAX
    ];

    char name[
        DIGIT_MODULE_NAME_MAX
    ];


    unsigned int version_major;

    unsigned int version_minor;

    unsigned int version_patch;


    unsigned int required_core_api_major;

    unsigned int required_core_api_minor;


    digit_module_qualify_fn qualify;

    digit_module_start_fn start;

    digit_module_stop_fn stop;

} digit_module_descriptor_t;


const char*
digit_module_state_string(
    digit_module_state_t state
);


const char*
digit_module_result_string(
    digit_module_result_t result
);


#endif