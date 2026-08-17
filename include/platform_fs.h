/**
 * @file platform_fs.h
 * @brief Platform filesystem interface for STN-LABZ Digit.
 *
 * This interface isolates operating-system-specific filesystem operations
 * from Digit Core.
 */

#ifndef STN_LABZ_PLATFORM_FS_H
#define STN_LABZ_PLATFORM_FS_H

#include <stddef.h>

/**
 * @brief Callback invoked for each regular file discovered in a directory.
 *
 * @param filename File name only, excluding the parent directory path.
 * @param context Caller-provided context pointer.
 *
 * @return 0 to continue enumeration, non-zero to stop enumeration.
 */
typedef int (*digit_platform_file_callback_t)(
    const char *filename,
    void *context
);

/**
 * @brief Enumerates regular files in a directory.
 *
 * Platform implementations enumerate only regular files and do not recurse
 * into subdirectories.
 *
 * @param directory Directory to enumerate.
 * @param callback Callback invoked for each discovered regular file.
 * @param context Caller-provided callback context.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_platform_enumerate_files(
    const char *directory,
    digit_platform_file_callback_t callback,
    void *context
);

/**
 * @brief Reads a file into caller-provided bounded memory.
 *
 * The function will not read a file larger than the supplied buffer.
 * One byte is reserved for a terminating null character.
 *
 * @param directory Parent directory containing the file.
 * @param filename File name to read.
 * @param buffer Destination buffer.
 * @param capacity Total destination buffer capacity.
 * @param bytes_read Receives the number of file bytes read.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_platform_read_file(
    const char *directory,
    const char *filename,
    char *buffer,
    size_t capacity,
    size_t *bytes_read
);

#endif /* STN_LABZ_PLATFORM_FS_H */