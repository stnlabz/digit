#include <stdio.h>
#include <string.h>

#include "hotload.h"

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
    digit_hotload_t hotload;
    digit_hotload_t empty;

    digit_module_manager_init(&manager, NULL);
    digit_hotload_init(&hotload, &manager);
    memset(&empty, 0, sizeof(empty));

    CHECK(hotload.manager == &manager,
          "hotload retains module manager");
    CHECK(hotload.count == 0,
          "hotload starts with empty snapshot");
    CHECK(digit_hotload_snapshot(NULL) == 0,
          "snapshot rejects null watcher");
    CHECK(digit_hotload_poll(NULL) == -1,
          "poll rejects null watcher");
    CHECK(digit_hotload_poll(&empty) == -1,
          "poll rejects watcher without manager");
    CHECK(digit_module_manager_path(&manager) != NULL,
          "watcher manager exposes module path");
    CHECK(DIGIT_HOTLOAD_MAX_MODULES == STNLABZ_MODULE_REGISTRY_MAX,
          "watch capacity matches ABI registry capacity");
    CHECK(DIGIT_HOTLOAD_PATH_MAX == STNLABZ_MODULE_LOADER_PATH_MAX,
          "watch path capacity matches ABI loader");
    CHECK(sizeof(hotload.files[0].module_id) == STNLABZ_MODULE_ID_MAX,
          "watch identity capacity matches ABI");
    CHECK(sizeof(hotload.files[0].path) == STNLABZ_MODULE_LOADER_PATH_MAX,
          "watch file path capacity matches ABI");

    printf("\nHotload watcher tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
