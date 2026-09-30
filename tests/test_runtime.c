#include <stdio.h>
#include <string.h>

#include "runtime.h"

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
    digit_runtime_t runtime;

    memset(&manager, 0, sizeof(manager));
    memset(&runtime, 0, sizeof(runtime));

    digit_runtime_init(&runtime, &manager);

    CHECK(runtime.modules == &manager,
          "runtime retains module manager");
    CHECK(runtime.running == 0,
          "runtime starts stopped");
    CHECK(runtime.hotload.manager == &manager,
          "runtime initializes hotload watcher");
    CHECK(runtime.hotload.count == 0,
          "runtime hotload snapshot starts empty");

    digit_runtime_init(NULL, &manager);
    CHECK(1,
          "runtime init tolerates null runtime");

    digit_runtime_init(&runtime, NULL);
    CHECK(runtime.modules == NULL,
          "runtime can represent missing module manager");
    CHECK(digit_runtime_run(NULL) == 0,
          "runtime rejects null runtime");
    CHECK(digit_runtime_run(&runtime) == 0,
          "runtime rejects missing module manager");

    digit_runtime_init(&runtime, &manager);
    digit_runtime_request_stop();
    CHECK(runtime.modules == &manager,
          "stop request does not corrupt runtime state");
    CHECK(runtime.running == 0,
          "stop request leaves inactive runtime stopped");

    printf("\nRuntime tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
