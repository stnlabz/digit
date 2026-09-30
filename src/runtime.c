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
    digit_hotload_init(&runtime->hotload, modules);
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

            runtime->running = 0;
            return 0;
        }

        if (digit_stop_requested)
        {
            break;
        }

        if (digit_hotload_poll(&runtime->hotload) < 0)
        {
            runtime->running = 0;
            return 0;
        }
    }

    runtime->running = 0;
    return 1;
}
