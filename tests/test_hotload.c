#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
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

/* [AI:GPT-6 | 2026-10-08] Exercise the actual candidate promotion path
 * at the exhausted audit boundary, without touching installed modules. */
static void test_audit_capacity(void)
{
    char root[] = "/tmp/digit-hotload-XXXXXX";
    char dir[512], path[640];
    unsigned char buffer[8192];
    FILE *src = NULL, *dst = NULL;
    size_t n;
    digit_module_manager_t *manager = NULL;
    digit_hotload_t *hotload = NULL;
    const stnlabz_module_record_t *incumbent;
    int ready = 0;

    if (mkdtemp(root) == NULL) { CHECK(0, "audit fixture temporary directory"); return; }
    snprintf(dir, sizeof(dir), "%s/sacrificial", root);
    snprintf(path, sizeof(path), "%s/sacrificial.so", dir);
    if (mkdir(dir, 0700) == 0) {
        src = fopen("build/tests/modules/sacrificial/sacrificial.so", "rb");
        dst = fopen(path, "wb");
        if (src && dst) {
            ready = 1;
            while ((n = fread(buffer, 1, sizeof(buffer), src)) != 0)
                if (fwrite(buffer, 1, n, dst) != n) { ready = 0; break; }
            if (ferror(src)) ready = 0;
        }
    }
    if (src) fclose(src);
    if (dst && fclose(dst) != 0) ready = 0;
    CHECK(ready, "isolated qualified candidate fixture");
    if (!ready) goto cleanup;

    manager = calloc(1, sizeof(*manager));
    hotload = calloc(1, sizeof(*hotload));
    if (!manager || !hotload) { CHECK(0, "audit fixture allocation"); goto cleanup; }
    digit_module_manager_init(manager, NULL);
    snprintf(manager->modules_path, sizeof(manager->modules_path), "%s", root);
    manager->registry.count = 1;
    strcpy(manager->registry.modules[0].descriptor.id, "sacrificial");
    manager->registry.modules[0].descriptor.version_major = 0;
    manager->registry.modules[0].state = STNLABZ_MODULE_STATE_ACTIVE;
    manager->registry.audit_count = STNLABZ_MODULE_AUDIT_MAX - 3;
    digit_hotload_init(hotload, manager);
    CHECK(digit_hotload_poll(hotload) == 0, "audit exhaustion defers candidate");
    incumbent = stnlabz_module_registry_find(&manager->registry, "sacrificial");
    CHECK(incumbent != NULL, "incumbent remains registered");
    CHECK(incumbent && incumbent->state == STNLABZ_MODULE_STATE_ACTIVE,
          "incumbent remains active");
    CHECK(incumbent && incumbent->descriptor.version_major == 0,
          "incumbent version remains unchanged");
    CHECK(manager->registry.audit_count == STNLABZ_MODULE_AUDIT_MAX - 3,
          "failed admission consumes no ABI audit entries");
    CHECK(manager->loader.count == 0, "deferred candidate does not enter loader");
cleanup:
    free(hotload);
    free(manager);
    unlink(path);
    rmdir(dir);
    rmdir(root);
}

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
