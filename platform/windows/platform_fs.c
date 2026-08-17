/**
 * @file platform_fs.c
 * @brief Windows filesystem implementation for STN-LABZ Digit.
 *
 * Windows-specific filesystem behavior is isolated within the platform
 * layer so Digit Core does not depend directly upon Win32 APIs.
 */

#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "platform_fs.h"

/**
 * @brief Maximum Windows path buffer used by this implementation.
 */
#define DIGIT_WINDOWS_PATH_MAX 1024U

int digit_platform_enumerate_files(
    const char *directory,
    digit_platform_file_callback_t callback,
    void *context)
{
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    char search_pattern[DIGIT_WINDOWS_PATH_MAX];
    int written;

    if (directory == NULL || callback == NULL)
    {
        return -1;
    }

    written = snprintf(
        search_pattern,
        sizeof(search_pattern),
        "%s\\*",
        directory
    );

    if (written < 0 ||
        (size_t)written >= sizeof(search_pattern))
    {
        return -1;
    }

    find_handle = FindFirstFileA(
        search_pattern,
        &find_data
    );

    if (find_handle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    do
    {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            continue;
        }

        if (callback(
                find_data.cFileName,
                context) != 0)
        {
            FindClose(find_handle);
            return 1;
        }

    } while (FindNextFileA(
                 find_handle,
                 &find_data) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        FindClose(find_handle);
        return -1;
    }

    FindClose(find_handle);

    return 0;
}

int digit_platform_read_file(
    const char *directory,
    const char *filename,
    char *buffer,
    size_t capacity,
    size_t *bytes_read)
{
    FILE *file;
    char full_path[DIGIT_WINDOWS_PATH_MAX];
    long file_size;
    size_t read_count;
    int written;

    if (directory == NULL ||
        filename == NULL ||
        buffer == NULL ||
        bytes_read == NULL ||
        capacity < 2U)
    {
        return -1;
    }

    *bytes_read = 0U;
    buffer[0] = '\0';

    written = snprintf(
        full_path,
        sizeof(full_path),
        "%s\\%s",
        directory,
        filename
    );

    if (written < 0 ||
        (size_t)written >= sizeof(full_path))
    {
        return -1;
    }

    if (fopen_s(
            &file,
            full_path,
            "rb") != 0 ||
        file == NULL)
    {
        return -1;
    }

    if (fseek(
            file,
            0L,
            SEEK_END) != 0)
    {
        fclose(file);
        return -1;
    }

    file_size = ftell(file);

    if (file_size < 0)
    {
        fclose(file);
        return -1;
    }

    /*
     * One byte is reserved for a null terminator.
     */
    if ((size_t)file_size >= capacity)
    {
        fclose(file);
        return -1;
    }

    if (fseek(
            file,
            0L,
            SEEK_SET) != 0)
    {
        fclose(file);
        return -1;
    }

    read_count = fread(
        buffer,
        1U,
        (size_t)file_size,
        file
    );

    if (read_count != (size_t)file_size)
    {
        fclose(file);
        return -1;
    }

    buffer[read_count] = '\0';
    *bytes_read = read_count;

    fclose(file);

    return 0;
}