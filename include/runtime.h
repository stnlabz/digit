#ifndef DIGIT_RUNTIME_H
#define DIGIT_RUNTIME_H

#include "hotload.h"
#include "module_manager.h"

typedef enum
{
    DIGIT_OPERATIONAL_NORMAL = 0,
    DIGIT_OPERATIONAL_SAFE_MODE = 1,
    DIGIT_OPERATIONAL_COMPANY_PRESERVATION = 2
} digit_operational_state_t;

typedef enum
{
    DIGIT_FAULT_AUTHORITY = 0,
    DIGIT_FAULT_INTEGRITY = 1,
    DIGIT_FAULT_SYSTEM = 2,
    DIGIT_FAULT_MISSION = 3,
    DIGIT_FAULT_TRUTHFULNESS = 4,
    DIGIT_FAULT_UNAUTHORIZED_CONDITION = 5
} digit_fault_t;

typedef struct
{
    digit_module_manager_t *modules;
    digit_hotload_t hotload;
    int running;
    digit_operational_state_t operational_state;
    digit_fault_t preservation_cause;
    int preservation_cause_set;
} digit_runtime_t;

void digit_runtime_init(
    digit_runtime_t *runtime,
    digit_module_manager_t *modules
);

int digit_runtime_run(
    digit_runtime_t *runtime
);

void digit_runtime_request_stop(void);

/*
 * Core-only operational-state controls.
 * A qualifying fault always enters Company Preservation through Safe Mode.
 * Normal operation can be restored only by an explicit authorized human
 * release supplied to digit_runtime_human_release().
 */
int digit_runtime_report_fault(
    digit_runtime_t *runtime,
    digit_fault_t fault
);

int digit_runtime_human_release(
    digit_runtime_t *runtime,
    int authorized_human_release
);

digit_operational_state_t digit_runtime_operational_state(
    const digit_runtime_t *runtime
);

int digit_runtime_normal_operations_allowed(
    const digit_runtime_t *runtime
);

#endif
