#include <stdio.h>
#include <string.h>

#include "corpus_builder.h"

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
    digit_corpus_builder_result_t result;

    memset(&result, 0, sizeof(result));
    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "corpus_builder") == 0, "module identity is corpus_builder");
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 0 && descriptor->version_patch == 1, "internal version is 1.0.1");
    check(descriptor != NULL && descriptor->qualify != NULL, "qualification callback exists");
    check(descriptor != NULL && descriptor->qualify(&qualification) == STNLABZ_MODULE_OK, "qualification executes");
    check(qualification.tests_executed >= STNLABZ_MODULE_MIN_TESTS, "required test count is reported");
    check(qualification.tests_passed == qualification.tests_executed, "all qualification tests pass");
    check(qualification.tests_failed == 0, "no qualification tests fail");
    check(qualification.negative_test_executed && qualification.negative_test_passed, "negative validation passes");
    check(descriptor != NULL && descriptor->start(NULL) == STNLABZ_MODULE_ERR_INVALID_ARGUMENT, "start rejects null host");

    printf("\nCorpus Builder module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
