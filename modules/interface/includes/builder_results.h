#ifndef DIGIT_INTERFACE_BUILDER_RESULTS_H
#define DIGIT_INTERFACE_BUILDER_RESULTS_H
/* [AI:GPT-6 | 2026-10-08] 1.5.3: validate builder service responses. */
typedef struct { int candidate; int stored; unsigned int confidence; char category[64]; char record_id[65]; char reason[256]; } interface_builder_result_t;
int digit_interface_builder_result_valid(const interface_builder_result_t *result);
#endif
