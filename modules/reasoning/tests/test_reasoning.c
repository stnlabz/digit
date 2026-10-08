#include <stdio.h>
#include <string.h>

#include "reasoning.h"

static unsigned int executed = 0;
static unsigned int failed = 0;

static void check(int condition, const char *name)
{
    ++executed;
    if (condition) printf("PASS %02u - %s\n", executed, name);
    else { ++failed; printf("FAIL %02u - %s\n", executed, name); }
}

int main(void)
{
    digit_reasoning_result_t result;
    const stnlabz_module_descriptor_t *descriptor = stnlabz_module_get_descriptor();

    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "reasoning") == 0, "module identity is reasoning");
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 0 && descriptor->version_patch == 7, "internal version is 1.0.7");
    check(descriptor != NULL && descriptor->qualify != NULL, "qualification callback exists");
    check(descriptor != NULL && descriptor->qualify(NULL) == STNLABZ_MODULE_ERR_INVALID_ARGUMENT, "qualification rejects null result");
    check(digit_reasoning_evaluate("Digit module qualification must pass before load.", &result) && result.relevance == DIGIT_RELEVANCE_RELEVANT && result.category == DIGIT_CONTEXT_RULE, "Digit must requirement is classified as rule");
    check(digit_reasoning_evaluate("SOURCE:\nDigit will continue normal mission execution when a non-critical optional module becomes unavailable.\n\nCONTEXT:\nThe module is critical and all other modules are at risk.", &result) && result.category == DIGIT_CONTEXT_RULE && result.confidence == 95U, "authoritative source wins over contradictory advisory context");
    check(digit_reasoning_evaluate("A module uses an ABI.", &result) && result.relevance == DIGIT_RELEVANCE_UNCERTAIN, "generic engineering context remains uncertain");
    check(digit_reasoning_evaluate("coffee tastes terrible today", &result) && result.relevance != DIGIT_RELEVANCE_RELEVANT, "unrelated conversation is not promoted to relevant");
    check(!digit_reasoning_evaluate(NULL, &result) && !digit_reasoning_evaluate("", &result) && !digit_reasoning_evaluate("Digit", NULL), "invalid reasoning input is rejected");

    printf("\nReasoning module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
