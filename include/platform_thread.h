/**
 * @file platform_thread.h
 * @brief Platform thread abstraction for STN-LABZ Digit.
 *
 * Core components use this interface rather than operating-system thread
 * primitives directly.
 */

#ifndef STN_LABZ_PLATFORM_THREAD_H
#define STN_LABZ_PLATFORM_THREAD_H

/**
 * @brief Opaque platform thread handle.
 */
typedef struct digit_platform_thread
{
    void *handle;
} digit_platform_thread_t;

/**
 * @brief Platform-independent thread entry function.
 *
 * @param context Caller-provided context.
 *
 * @return Thread result code.
 */
typedef unsigned int (*digit_platform_thread_function_t)(
    void *context
);

/**
 * @brief Creates one worker thread.
 *
 * @param thread Thread storage.
 * @param function Thread entry function.
 * @param context Caller-provided context.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_platform_thread_create(
    digit_platform_thread_t *thread,
    digit_platform_thread_function_t function,
    void *context
);

/**
 * @brief Waits for a worker thread to terminate.
 *
 * @param thread Thread storage.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_platform_thread_join(
    digit_platform_thread_t *thread
);

/**
 * @brief Releases a terminated thread handle.
 *
 * @param thread Thread storage.
 */
void digit_platform_thread_close(
    digit_platform_thread_t *thread
);

#endif /* STN_LABZ_PLATFORM_THREAD_H */