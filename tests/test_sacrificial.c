#include <stdio.h>
#include <string.h>

#include "module_loader.h"

static int failures = 0;
static int tests = 0;

#define CHECK(condition, name) \
    do { \
        ++tests; \
        if (condition) { printf("PASS %02d - %s\n", tests, name); } \
        else { printf("FAIL %02d - %s\n", tests, name); ++failures; } \
    } while (0)

int main(void)
{
    stnlabz_module_loader_t loader;
    const stnlabz_module_descriptor_t *descriptor = NULL;
    stnlabz_module_qualification_result_t result;
    stnlabz_module_loader_result_t load_result;

    stnlabz_module_loader_init(&loader);
    memset(&result, 0, sizeof(result));

    load_result = stnlabz_module_loader_load(
        &loader,
        "sacrificial",
        "build/modules/sacrificial/sacrificial.so",
        &descriptor
    );

    CHECK(load_result == STNLABZ_MODULE_LOADER_OK,
          "sacrificial shared object loads");
    CHECK(descriptor != NULL,
          "sacrificial descriptor is exported");
    CHECK(descriptor != NULL && strcmp(descriptor->id, "sacrificial") == 0,
          "sacrificial identity matches directory identity");
    CHECK(descriptor != NULL && descriptor->version_major == 1 &&
          descriptor->version_minor == 0 && descriptor->version_patch == 0,
          "sacrificial internal version is 1.0.0");
    CHECK(descriptor != NULL && descriptor->qualify != NULL,
          "sacrificial qualification callback exists");

    if (descriptor != NULL && descriptor->qualify != NULL)
    {
        CHECK(descriptor->qualify(&result) == STNLABZ_MODULE_OK,
              "sacrificial qualification executes");
    }
    else
    {
        CHECK(0, "sacrificial qualification executes");
    }

    CHECK(result.tests_executed >= STNLABZ_MODULE_MIN_TESTS,
          "sacrificial executes required test count");
    CHECK(result.tests_failed == 0 && result.tests_passed == result.tests_executed,
          "sacrificial required tests pass");
    CHECK(result.negative_test_executed && result.negative_test_passed,
          "sacrificial negative validation passes");

    CHECK(stnlabz_module_loader_unload(&loader, "sacrificial") ==
          STNLABZ_MODULE_LOADER_OK,
          "sacrificial shared object unloads");

    printf("\nSacrificial module tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
