/**
 * @file platform_console.c
 * @brief Windows console implementation for STN-LABZ Digit.
 *
 * Configures the Windows console to use UTF-8 for input and output.
 *
 * Windows-specific console behavior remains isolated within the
 * platform layer.
 */

#include <windows.h>

#include "platform_console.h"

int digit_platform_console_init(void)
{
    if (SetConsoleOutputCP(CP_UTF8) == 0)
    {
        return -1;
    }

    if (SetConsoleCP(CP_UTF8) == 0)
    {
        return -1;
    }

    return 0;
}