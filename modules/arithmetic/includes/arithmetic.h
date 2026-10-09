#ifndef DIGIT_ARITHMETIC_H
#define DIGIT_ARITHMETIC_H

#include <stdint.h>
#include "module.h"

#define DIGIT_ARITHMETIC_SERVICE "arithmetic.evaluate"
#define DIGIT_ARITHMETIC_EXPRESSION_MAX 256

typedef enum {
    DIGIT_ARITHMETIC_OK=0,
    DIGIT_ARITHMETIC_INVALID=1,
    DIGIT_ARITHMETIC_DIVIDE_BY_ZERO=2,
    DIGIT_ARITHMETIC_OUT_OF_RANGE=3
} digit_arithmetic_status_t;

typedef struct {
    char expression[DIGIT_ARITHMETIC_EXPRESSION_MAX];
} digit_arithmetic_request_t;

typedef struct {
    digit_arithmetic_status_t status;
    int64_t left;
    int64_t right;
    int64_t value;
    int64_t remainder;
    char operation;
    /* [AI:GPT-6 | 2026-10-09] Exact fixed-point result when decimal operands are used. */
    char decimal_answer[128];
} digit_arithmetic_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);
#endif
