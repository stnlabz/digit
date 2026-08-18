/**
 * @file digit_authority.h
 * @brief Human-authority boundary for STN-LABZ Digit Core.
 *
 * Human authority is part of Digit's Core safety nucleus.
 *
 * Digit does not independently grant, expand, infer, or manufacture
 * human authorization.
 */

#ifndef STN_LABZ_DIGIT_AUTHORITY_H
#define STN_LABZ_DIGIT_AUTHORITY_H

/**
 * @brief Human-authority state.
 */
typedef enum digit_authority_state
{
    DIGIT_AUTHORITY_UNKNOWN = 0,
    DIGIT_AUTHORITY_NOT_REQUIRED,
    DIGIT_AUTHORITY_REQUIRED,
    DIGIT_AUTHORITY_GRANTED,
    DIGIT_AUTHORITY_DENIED
} digit_authority_state_t;

/**
 * @brief Result of a human-authority evaluation.
 */
typedef struct digit_authority_result
{
    digit_authority_state_t state;

    int human_authority_required;
    int human_authority_established;

} digit_authority_result_t;

/**
 * @brief Initializes the Core human-authority boundary.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_authority_init(void);

/**
 * @brief Evaluates a human-authority requirement.
 *
 * When human authority is required, Digit will not treat the operation
 * as authorized unless explicit human authorization has been established.
 *
 * @param authority_required Non-zero when human authority is required.
 * @param human_approved Non-zero only when explicit human approval has
 *        been established by an applicable authorized process.
 * @param result Receives the deterministic authority result.
 *
 * @return 0 when evaluation completes, non-zero on invalid arguments
 *         or inactive authority boundary.
 */
int digit_authority_evaluate(
    int authority_required,
    int human_approved,
    digit_authority_result_t *result
);

/**
 * @brief Determines whether the authority boundary is active.
 *
 * @return 1 when initialized, otherwise 0.
 */
int digit_authority_is_ready(void);

/**
 * @brief Withdraws active authority-boundary state.
 */
void digit_authority_shutdown(void);

#endif /* STN_LABZ_DIGIT_AUTHORITY_H */