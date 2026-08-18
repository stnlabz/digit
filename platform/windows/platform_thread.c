/**
 * @file platform_thread.c
 * @brief Windows thread implementation for STN-LABZ Digit.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>
#include <stddef.h>
#include <stdlib.h>

#include "platform_thread.h"

/**
 * @brief Internal Windows thread-start information.
 */
typedef struct digit_windows_thread_start
{
    digit_platform_thread_function_t function;
    void *context;
} digit_windows_thread_start_t;

/**
 * @brief Windows CRT-compatible thread entry wrapper.
 *
 * @param argument Internal thread-start information.
 *
 * @return Worker result.
 */
static unsigned int __stdcall digit_windows_thread_entry(
    void *argument)
{
    digit_windows_thread_start_t *start;
    digit_platform_thread_function_t function;
    void *context;
    unsigned int result;

    if (argument == NULL)
    {
        return 1U;
    }

    start =
        (digit_windows_thread_start_t *)argument;

    function = start->function;
    context = start->context;

    free(start);

    if (function == NULL)
    {
        return 1U;
    }

    result =
        function(context);

    return result;
}

int digit_platform_thread_create(
    digit_platform_thread_t *thread,
    digit_platform_thread_function_t function,
    void *context)
{
    digit_windows_thread_start_t *start;
    uintptr_t handle;

    if (thread == NULL ||
        function == NULL)
    {
        return -1;
    }

    if (thread->handle != NULL)
    {
        return -1;
    }

    start =
        (digit_windows_thread_start_t *)malloc(
            sizeof(*start)
        );

    if (start == NULL)
    {
        return -1;
    }

    start->function = function;
    start->context = context;

    handle =
        _beginthreadex(
            NULL,
            0U,
            digit_windows_thread_entry,
            start,
            0U,
            NULL
        );

    if (handle == 0U)
    {
        free(start);

        return -1;
    }

    thread->handle =
        (void *)handle;

    return 0;
}

int digit_platform_thread_join(
    digit_platform_thread_t *thread)
{
    DWORD result;

    if (thread == NULL ||
        thread->handle == NULL)
    {
        return -1;
    }

    result =
        WaitForSingleObject(
            (HANDLE)thread->handle,
            INFINITE
        );

    if (result != WAIT_OBJECT_0)
    {
        return -1;
    }

    return 0;
}

void digit_platform_thread_close(
    digit_platform_thread_t *thread)
{
    if (thread == NULL)
    {
        return;
    }

    if (thread->handle != NULL)
    {
        (void)CloseHandle(
            (HANDLE)thread->handle
        );

        thread->handle = NULL;
    }
}