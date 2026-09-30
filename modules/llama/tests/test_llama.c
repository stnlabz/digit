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

    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "llama") == 0, "module identity is llama");
    check(descriptor != NULL && descriptor->version_major == 1, "major version is 1");
    check(descriptor != NULL && descriptor->version_minor == 0, "minor version is 0");
    check(descriptor != NULL && descriptor->version_patch == 8, "patch version is 8");
    check(descriptor != NULL && descriptor->qualify != NULL, "qualification callback exists");
    check(descriptor != NULL && descriptor->qualify(&result) == STNLABZ_MODULE_OK, "qualification executes");
    check(result.tests_executed >= STNLABZ_MODULE_MIN_TESTS, "required test count is met");
    check(result.tests_failed == 0 && result.tests_passed == result.tests_executed, "required tests pass");
    check(result.negative_test_executed && result.negative_test_passed, "negative validation passes");

    printf("\nLlama module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
