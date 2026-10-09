#include <stdio.h>
#include <string.h>

#include "interface.h"

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
    stnlabz_module_qualification_result_t qualification;
    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "interface") == 0, "module identity is interface");
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 2 && descriptor->version_patch == 10, "internal version is 1.2.10");
    check(descriptor != NULL && descriptor->qualify != NULL, "qualification callback exists");
    check(descriptor != NULL && descriptor->qualify(&qualification) == STNLABZ_MODULE_OK, "qualification executes");
    check(qualification.tests_executed >= STNLABZ_MODULE_MIN_TESTS, "required test count is reported");
    check(qualification.tests_passed == qualification.tests_executed && qualification.tests_failed == 0, "required tests pass");
    check(qualification.negative_test_executed && qualification.negative_test_passed, "negative validation passes");
    check(descriptor != NULL && descriptor->start(NULL) == STNLABZ_MODULE_ERR_INVALID_ARGUMENT, "start rejects null host");
    check(strcmp(DIGIT_INTERFACE_DEFAULT_HOST, "127.0.0.1") == 0 && DIGIT_INTERFACE_DEFAULT_PORT == 8081, "interface binds loopback until authenticated gateway exists");
    printf("\nInterface module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
