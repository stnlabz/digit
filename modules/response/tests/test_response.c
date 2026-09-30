#include <stdio.h>
#include <string.h>

#include "response.h"

static unsigned int executed = 0;
static unsigned int failed = 0;

static void check(int condition, const char *name)
{
    ++executed;
    if (condition) {
        printf("PASS %02u - %s\n", executed, name);
    } else {
        ++failed;
        printf("FAIL %02u - %s\n", executed, name);
    }
}

int main(void)
{
    const stnlabz_module_descriptor_t *descriptor = stnlabz_module_get_descriptor();
    stnlabz_module_qualification_result_t qualification;
    digit_response_request_t request;
    digit_response_result_t result;

    memset(&request, 0, sizeof(request));
    memset(&result, 0, sizeof(result));
    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "response") == 0,
          "module identity is response");
    check(descriptor != NULL && descriptor->version_major == 1 &&
          descriptor->version_minor == 0 && descriptor->version_patch == 10,
          "internal version is 1.0.10");
    check(descriptor != NULL && descriptor->qualify != NULL,
          "qualification callback exists");
    check(descriptor != NULL &&
          descriptor->qualify(&qualification) == STNLABZ_MODULE_OK,
          "qualification executes");
    check(qualification.tests_executed >= STNLABZ_MODULE_MIN_TESTS,
          "required test count is reported");
    check(qualification.tests_passed == qualification.tests_executed &&
          qualification.tests_failed == 0,
          "required tests pass");
    check(qualification.negative_test_executed && qualification.negative_test_passed,
          "negative validation passes");
    check(sizeof(request.question) == DIGIT_RESPONSE_QUESTION_MAX,
          "question contract is bounded");
    check(sizeof(result.answer) == DIGIT_RESPONSE_ANSWER_MAX,
          "answer contract is bounded");
    printf("\nResponse module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
