/**
 * @file digit_result.h
 * @brief Result and epistemic-state interface for STN-LABZ Digit Core.
 *
 * Result and epistemic states are part of Digit's Core safety nucleus.
 *
 * Digit preserves UNKNOWN rather than manufacturing certainty.
 * Inference remains distinguishable from established factual state.
 */

#ifndef STN_LABZ_DIGIT_RESULT_H
#define STN_LABZ_DIGIT_RESULT_H

/**
 * @brief Deterministic Core result state.
 */
typedef enum digit_result_state
{
    DIGIT_RESULT_UNKNOWN = 0,
    DIGIT_RESULT_PASS,
    DIGIT_RESULT_FAIL
} digit_result_state_t;

/**
 * @brief Epistemic state associated with information used by Digit.
 */
typedef enum digit_epistemic_state
{
    DIGIT_EPISTEMIC_UNKNOWN = 0,
    DIGIT_EPISTEMIC_ESTABLISHED,
    DIGIT_EPISTEMIC_INFERRED
} digit_epistemic_state_t;

/**
 * @brief Combined Core result representation.
 */
typedef struct digit_result
{
    digit_result_state_t result;
    digit_epistemic_state_t epistemic;

} digit_result_t;

/**
 * @brief Initializes the result / epistemic-state Core component.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_result_init(void);

/**
 * @brief Initializes a result object to the fail-safe UNKNOWN state.
 *
 * @param result Result object to initialize.
 *
 * @return 0 on success, non-zero on invalid arguments or inactive Core
 *         component.
 */
int digit_result_set_unknown(
    digit_result_t *result
);

/**
 * @brief Establishes a deterministic result from established evidence.
 *
 * PASS and FAIL may be represented as established only when the caller has
 * already established the applicable evidence through the governing
 * validation boundary.
 *
 * @param result Result object.
 * @param state DIGIT_RESULT_PASS or DIGIT_RESULT_FAIL.
 *
 * @return 0 on success, non-zero on invalid state, arguments, or inactive
 *         component.
 */
int digit_result_set_established(
    digit_result_t *result,
    digit_result_state_t state
);

/**
 * @brief Represents an explicitly labeled inference.
 *
 * An inferred state does not become established factual state.
 *
 * The operational result remains UNKNOWN because inference alone does not
 * establish PASS or FAIL.
 *
 * @param result Result object.
 *
 * @return 0 on success, non-zero on invalid arguments or inactive component.
 */
int digit_result_set_inferred(
    digit_result_t *result
);

/**
 * @brief Determines whether a result represents established evidence.
 *
 * @param result Result object.
 *
 * @return 1 when epistemic state is ESTABLISHED, otherwise 0.
 */
int digit_result_is_established(
    const digit_result_t *result
);

/**
 * @brief Determines whether the result component is active.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_result_is_ready(void);

/**
 * @brief Withdraws active result / epistemic-state Core state.
 */
void digit_result_shutdown(void);

#endif /* STN_LABZ_DIGIT_RESULT_H */