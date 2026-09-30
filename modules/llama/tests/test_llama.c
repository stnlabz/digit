#include <stdio.h>
#include <string.h>

#include "llama.h"
#include "module.h"

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
    const stnlabz_module_descriptor_t *descriptor = stnlabz_module_get_descriptor();
    stnlabz_module_qualification_result_t result;
    digit_llama_generate_request_t request;
    digit_llama_generate_result_t response;

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));

    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "llama") == 0, "module identity is llama");
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 1 && descriptor->version_patch == 0, "internal version is 1.1.0");
    check(descriptor != NULL && descriptor->qualify != NULL, "qualification callback exists");
    check(descriptor != NULL && descriptor->qualify(&result) == STNLABZ_MODULE_OK, "qualification executes");
    check(result.tests_executed >= STNLABZ_MODULE_MIN_TESTS, "required test count is met");
    check(result.tests_failed == 0 && result.tests_passed == result.tests_executed, "required tests pass");
    check(result.negative_test_executed && result.negative_test_passed, "negative validation passes");
    check(sizeof(request.prompt) == DIGIT_LLAMA_PROMPT_MAX, "bounded generation request contract is fixed");
    check(sizeof(response.text) == DIGIT_LLAMA_GENERATED_MAX, "bounded generation response contract is fixed");

    printf("\nLlama module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
