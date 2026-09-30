#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <time.h>

#include "runtime.h"

#define DIGIT_RUNTIME_POLL_SECONDS 1

static volatile sig_atomic_t digit_stop_requested = 0;

static void digit_runtime_signal_handler(int signal_number)
{
    (void)signal_number;
    digit_stop_requested = 1;
}

void digit_runtime_request_stop(void)
{
    digit_stop_requested = 1;
}

void digit_runtime_init(
    digit_runtime_t *runtime,
    digit_module_manager_t *modules
)
{
    if (runtime == NULL)
    {
        return;
    }

    runtime->modules = modules;
    runtime->running = 0;
    runtime->operational_state = DIGIT_OPERATIONAL_NORMAL;
    runtime->preservation_cause = DIGIT_FAULT_SYSTEM;
    runtime->preservation_cause_set = 0;
    digit_hotload_init(&runtime->hotload, modules);
}

int digit_runtime_report_fault(
    digit_runtime_t *runtime,
    digit_fault_t fault
)
{
    if (runtime == NULL)
    {
        return 0;
    }

    /*
     * Safe Mode is mandatory for a qualifying fault.  The transition into
     * Company Preservation is Core-owned and cannot be bypassed by a module.
     */
    runtime->operational_state = DIGIT_OPERATIONAL_SAFE_MODE;
    runtime->preservation_cause = fault;
    runtime->preservation_cause_set = 1;

    /*
     * Company Preservation is the only operational mission permitted after
     * Safe Mode entry.  Normal engineering activity remains unavailable.
     */
    runtime->operational_state = DIGIT_OPERATIONAL_COMPANY_PRESERVATION;
    return 1;
}

int digit_runtime_human_release(
    digit_runtime_t *runtime,
    int authorized_human_release
)
{
    if (runtime == NULL || !authorized_human_release)
    {
        return 0;
    }

    if (runtime->operational_state != DIGIT_OPERATIONAL_COMPANY_PRESERVATION)
    {
        return 0;
    }

    runtime->operational_state = DIGIT_OPERATIONAL_NORMAL;
    runtime->preservation_cause_set = 0;
    return 1;
}

digit_operational_state_t digit_runtime_operational_state(
    const digit_runtime_t *runtime
)
{
    if (runtime == NULL)
    {
        return DIGIT_OPERATIONAL_COMPANY_PRESERVATION;
    }

    return runtime->operational_state;
}

int digit_runtime_normal_operations_allowed(
    const digit_runtime_t *runtime
)
{
    return runtime != NULL &&
        runtime->operational_state == DIGIT_OPERATIONAL_NORMAL;
}

int digit_runtime_run(
    digit_runtime_t *runtime
)
{
    struct sigaction action;
    struct timespec delay;

    if (runtime == NULL || runtime->modules == NULL)
    {
        return 0;
    }

    action.sa_handler = digit_runtime_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0)
    {
        return 0;
    }

    if (!digit_hotload_snapshot(&runtime->hotload))
    {
        return 0;
    }

    delay.tv_sec = DIGIT_RUNTIME_POLL_SECONDS;
    delay.tv_nsec = 0;
    digit_stop_requested = 0;
    runtime->running = 1;

    while (!digit_stop_requested)
    {
        struct timespec remaining = delay;

        while (nanosleep(&remaining, &remaining) != 0)
        {
            if (errno == EINTR)
            {
                if (digit_stop_requested)
                {
                    break;
                }
                continue;
            }

            (void)digit_runtime_report_fault(runtime, DIGIT_FAULT_SYSTEM);
            runtime->running = 0;
            return 0;
        }

        if (digit_stop_requested)
        {
            break;
        }

        if (!digit_runtime_normal_operations_allowed(runtime))
        {
            /*
             * Company Preservation intentionally does not run normal module
             * hotload activity.  Core remains alive awaiting authorized human
             * intervention while preservation controls retain authority.
             */
            continue;
        }

        if (digit_hotload_poll(&runtime->hotload) < 0)
        {
            (void)digit_runtime_report_fault(runtime, DIGIT_FAULT_SYSTEM);
            continue;
        }
    }

    runtime->running = 0;
    return 1;
}
