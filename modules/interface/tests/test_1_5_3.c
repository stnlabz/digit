#include <stdio.h>
#include <string.h>
#include "builder_response.h"

/* [AI:GPT-6 | 2026-10-08] Interface 1.5.3 Builder response qualification tests. */
static unsigned int executed, failed;
static void check(int ok, const char *name)
{
    ++executed;
    printf("%s 1.5.3 %02u - %s\n", ok ? "PASS" : "FAIL", executed, name);
    if (!ok) ++failed;
}

static digit_interface_builder_result_t sample(void)
{
    digit_interface_builder_result_t r = {0};
    r.candidate = 1;
    r.stored = 1;
    r.confidence = 100;
    strcpy(r.category, "OPERATOR_LEARNED");
    strcpy(r.record_id, "DIGIT-0123456789abcdef");
    strcpy(r.reason, "Explicit operator learning committed to Corpus.");
    return r;
}

int main(void)
{
    digit_interface_builder_result_t r = sample();
    check(digit_interface_builder_result_valid(&r), "valid stored Builder result");
    r.stored = 0;
    check(digit_interface_builder_result_valid(&r), "valid duplicate candidate");
    r = sample(); r.candidate = 0;
    check(!digit_interface_builder_result_valid(&r), "stored without candidate rejected");
    r = sample(); r.candidate = 2;
    check(!digit_interface_builder_result_valid(&r), "nonboolean candidate rejected");
    r = sample(); r.stored = -1;
    check(!digit_interface_builder_result_valid(&r), "nonboolean stored rejected");
    r = sample(); r.confidence = 101;
    check(!digit_interface_builder_result_valid(&r), "out-of-range confidence rejected");
    r = sample(); r.record_id[7] = 'z';
    check(!digit_interface_builder_result_valid(&r), "malformed record ID rejected");
    r = sample(); r.record_id[10] = 0;
    check(!digit_interface_builder_result_valid(&r), "short record ID rejected");
    r = sample(); memset(r.record_id, 'A', sizeof(r.record_id));
    check(!digit_interface_builder_result_valid(&r), "unterminated record ID rejected");
    r = sample(); memset(r.category, 'A', sizeof(r.category));
    check(!digit_interface_builder_result_valid(&r), "unterminated category rejected");
    r = sample(); memset(r.reason, 'A', sizeof(r.reason));
    check(!digit_interface_builder_result_valid(&r), "unterminated reason rejected");
    r = sample(); r.reason[0] = 0;
    check(!digit_interface_builder_result_valid(&r), "empty reason rejected");
    r = sample(); r.category[0] = 0;
    check(!digit_interface_builder_result_valid(&r), "empty category rejected");
    r = sample(); r.candidate = 0; r.stored = 0; r.record_id[0] = 0;
    check(digit_interface_builder_result_valid(&r), "valid noncandidate result");
    r = sample(); r.candidate = 0; r.stored = 0;
    check(!digit_interface_builder_result_valid(&r), "noncandidate record ID rejected");
    check(!digit_interface_builder_result_valid(NULL), "null result rejected");
    printf("Interface 1.5.3 milestone: %u executed, %u failed\n", executed, failed);
    return failed != 0;
}
