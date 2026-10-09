#ifndef DIGIT_INTERFACE_BUILDER_RESPONSE_H
#define DIGIT_INTERFACE_BUILDER_RESPONSE_H

#include <stddef.h>

/* [AI:GPT-6 | 2026-10-08] Interface 1.5.3: validate the
 * existing Corpus Builder response without changing its ABI. */
typedef struct
{
    int candidate;
    int stored;
    unsigned int confidence;
    char category[64];
    char record_id[65];
    char reason[256];
} digit_interface_builder_result_t;

int digit_interface_builder_result_valid(
    const digit_interface_builder_result_t *result);

#endif
