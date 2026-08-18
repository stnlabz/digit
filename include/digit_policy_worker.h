/**
 * @file digit_policy_worker.h
 * @brief Policy worker interface for STN-LABZ Digit Core.
 *
 * The policy worker provides a bounded asynchronous execution boundary
 * for policy discovery, validation, and indexing work.
 *
 * The worker does not determine knowledge authorization.
 * Authorization remains governed by Digit Core.
 */

#ifndef STN_LABZ_DIGIT_POLICY_WORKER_H
#define STN_LABZ_DIGIT_POLICY_WORKER_H

#include "digit_policy_index.h"

/**
 * @brief Policy worker runtime state.
 */
typedef enum digit_policy_worker_state
{
    DIGIT_POLICY_WORKER_STOPPED = 0,
    DIGIT_POLICY_WORKER_IDLE,
    DIGIT_POLICY_WORKER_RUNNING,
    DIGIT_POLICY_WORKER_COMPLETE,
    DIGIT_POLICY_WORKER_FAILED
} digit_policy_worker_state_t;

/**
 * @brief Initializes the policy worker.
 *
 * Initialization establishes an IDLE worker.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_policy_worker_init(void);

/**
 * @brief Starts one policy worker job using a recognized policy index.
 *
 * The supplied index is copied into worker-owned job storage before
 * asynchronous execution begins.
 *
 * Index presence and candidate matching do not independently establish
 * approval, authorization, cryptographic integrity, Trust Chain validity,
 * or eligibility for consumption.
 *
 * Only one job may execute at a time.
 *
 * @param policy_index Recognized policy index for this worker job.
 *
 * @return 0 on successful start, non-zero when unavailable or invalid.
 */
int digit_policy_worker_start(
    const digit_policy_index_t *policy_index
);

/**
 * @brief Returns the current worker state.
 *
 * @return Current policy worker state.
 */
digit_policy_worker_state_t digit_policy_worker_get_state(void);

/**
 * @brief Resets a completed or failed worker to IDLE.
 *
 * A running worker cannot be reset.
 *
 * @return 0 on success, non-zero on invalid transition.
 */
int digit_policy_worker_reset(void);

/**
 * @brief Determines whether the worker component is initialized.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_policy_worker_is_ready(void);

/**
 * @brief Stops and withdraws the policy worker.
 *
 * If work is active, shutdown waits for the worker thread to terminate
 * before releasing its platform resources.
 */
void digit_policy_worker_shutdown(void);

#endif /* STN_LABZ_DIGIT_POLICY_WORKER_H */