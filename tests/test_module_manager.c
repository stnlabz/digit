#include <stdio.h>
#include <string.h>

#include "module_manager.h"
#include "audit.h"

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
    digit_module_manager_t manager;
    stnlabz_module_discovery_report_t report;
    stnlabz_module_result_t result;

    digit_module_manager_init(&manager, NULL);

    CHECK(digit_module_manager_count(&manager) == 0,
          "manager starts with zero modules");
    CHECK(digit_audit_lifecycle_count() == 0,
          "manager starts with empty audit trail");
    CHECK(manager.loader.count == 0,
          "loader starts empty");
    CHECK(digit_module_manager_path(&manager) != NULL,
          "module path accessor is available");

    result = digit_module_manager_discover(NULL, &report);
    CHECK(result == STNLABZ_MODULE_ERR_INVALID_ARGUMENT,
          "discovery rejects null manager");

    result = digit_module_manager_discover(&manager, NULL);
    CHECK(result == STNLABZ_MODULE_ERR_INVALID_ARGUMENT,
          "discovery rejects null report");

    result = digit_module_manager_qualify_and_activate(NULL);
    CHECK(result == STNLABZ_MODULE_ERR_INVALID_ARGUMENT,
          "qualification rejects null manager");

    result = digit_module_manager_qualify_and_activate(&manager);
    CHECK(result == STNLABZ_MODULE_OK,
          "empty registry requires no activation");

    CHECK(STNLABZ_MODULE_MIN_TESTS == 10,
          "ABI minimum module test requirement is ten");

    digit_module_manager_shutdown(&manager);
    CHECK(manager.loader.count == 0,
          "shutdown leaves loader empty");

    printf("\nModule manager tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
