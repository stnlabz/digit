#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

/* [AI:GPT-6 | 2026-10-08] Actual active-module replacement and
 * rejected start recovery; all artifacts remain under a private /tmp root. */
static int copy_fixture(const char *source, const char *target)
{
    FILE *in = fopen(source, "rb"), *out = NULL;
    unsigned char bytes[8192];
    size_t n;
    int ok = 1;
    if (in) out = fopen(target, "wb");
    if (!in || !out) ok = 0;
    if (ok) {
        while ((n = fread(bytes, 1, sizeof(bytes), in)) != 0)
            if (fwrite(bytes, 1, n, out) != n) { ok = 0; break; }
        if (ferror(in)) ok = 0;
    }
    if (in) fclose(in);
    if (out && fclose(out) != 0) ok = 0;
    return ok;
}

static void test_active_transition(int fail_start, int fail_stop, int fail_restart)
{
    char root[] = "/tmp/digit-swap-XXXXXX";
    char dir[512], bin[576], live[640], replacement[660];
    const char *updated = fail_start ?
        "build/tests/modules/sacrificial/sacrificial_fail.so" :
        "build/tests/modules/sacrificial/sacrificial_v2.so";
    digit_module_manager_t *manager = NULL;
    digit_hotload_t *watcher = NULL;
    const stnlabz_module_descriptor_t *descriptor = NULL;
    const stnlabz_module_record_t *record;
    const stnlabz_loaded_module_t *loaded;
    stnlabz_module_qualification_result_t qualification = {0};
    stnlabz_module_descriptor_t known = {0};
    int ready = 0;

    if (!mkdtemp(root)) { CHECK(0, "transition fixture root"); return; }
    snprintf(dir, sizeof(dir), "%s/sacrificial", root);
    snprintf(bin, sizeof(bin), "%s/bin", dir);
    snprintf(live, sizeof(live), "%s/sacrificial.so", bin);
    snprintf(replacement, sizeof(replacement), "%s.new", live);
    if (mkdir(dir, 0700) || mkdir(bin, 0700) ||
        !copy_fixture(fail_stop ? "build/tests/modules/sacrificial/sacrificial_stop_fail.so" :
                      fail_restart ? "build/tests/modules/sacrificial/sacrificial_restart_fail.so" :
                      "build/tests/modules/sacrificial/sacrificial.so", live))
        goto cleanup;

    manager = calloc(1, sizeof(*manager));
    watcher = calloc(1, sizeof(*watcher));
    if (!manager || !watcher) goto cleanup;
    digit_module_manager_init(manager, NULL);
    snprintf(manager->modules_path, sizeof(manager->modules_path), "%s", root);
    if (stnlabz_module_loader_load(&manager->loader, "sacrificial", live, &descriptor) != STNLABZ_MODULE_LOADER_OK)
        goto cleanup;
    if (stnlabz_module_registry_discover(&manager->registry, descriptor) != STNLABZ_MODULE_OK ||
        stnlabz_module_registry_verify(&manager->registry, "sacrificial") != STNLABZ_MODULE_OK)
        goto cleanup;
    qualification.tests_executed = 10;
    qualification.tests_passed = 10;
    qualification.negative_test_executed = 1;
    qualification.negative_test_passed = 1;
    if (stnlabz_module_registry_restore_qualification(&manager->registry, "sacrificial", &qualification) != STNLABZ_MODULE_OK ||
        stnlabz_module_registry_authorize_activation(&manager->registry, "sacrificial") != STNLABZ_MODULE_OK ||
        stnlabz_module_registry_activate(&manager->registry, "sacrificial") != STNLABZ_MODULE_OK ||
        descriptor->start(&manager->host) != STNLABZ_MODULE_OK)
        goto cleanup;
    strcpy(known.id, "sacrificial");
    known.version_major = 1;
    known.version_patch = fail_start ? 2U : 1U;
    if (!digit_qualification_record(&manager->qualifications, &known))
        goto cleanup;
    digit_hotload_init(watcher, manager);
    if (!digit_hotload_snapshot(watcher)) goto cleanup;
    if (!copy_fixture(updated, replacement) || rename(replacement, live) != 0) goto cleanup;
    /* Deterministic update detection independent of filesystem timestamp resolution. */
    watcher->files[0].modified_time = 0;
    ready = 1;
    CHECK(ready, fail_stop ? "failed-stop fixture prepared" :
          fail_start ? "failed-start fixture prepared" : "replacement fixture prepared");
    CHECK(digit_hotload_poll(watcher) == ((fail_start || fail_stop || fail_restart) ? 0 : 1),
          fail_stop ? "failed incumbent stop rejects replacement" :
          fail_start ? "failed candidate rejected" : "active replacement accepted");
    record = stnlabz_module_registry_find(&manager->registry, "sacrificial");
    CHECK(record && record->state == (fail_restart ? STNLABZ_MODULE_STATE_FAILED : STNLABZ_MODULE_STATE_ACTIVE),
          fail_restart ? "failed rollback never claims incumbent ACTIVE" :
                         "replacement preserves active module state");
    CHECK(record && record->descriptor.version_patch == ((fail_start || fail_stop || fail_restart) ? 0U : 1U),
          fail_start ? "incumbent version restored" : "new version active");
    loaded = stnlabz_module_loader_find(&manager->loader, "sacrificial");
    CHECK(loaded && loaded->descriptor &&
          loaded->descriptor->version_patch == ((fail_start || fail_stop || fail_restart) ? 0U : 1U),
          "loader matches registry version");
    CHECK(manager->loader.count == 1, "single module handle retained");
    CHECK(watcher->count == (fail_start ? 1U : 1U),
          "candidate snapshot remains valid");
cleanup:
    if (!ready) CHECK(0, fail_stop ? "failed-stop fixture prepared" :
          fail_start ? "failed-start fixture prepared" : "replacement fixture prepared");
    if (manager) stnlabz_module_loader_unload_all(&manager->loader);
    free(watcher);
    free(manager);
    unlink(replacement);
    unlink(live);
    rmdir(bin);
    rmdir(dir);
    rmdir(root);
}

/* [AI:GPT-6 | 2026-10-08] Hung qualification must not stop
 * the running incumbent or block Core's poll indefinitely. */
static void test_qualification_timeout(void)
{
    char root[] = "/tmp/digit-timeout-XXXXXX";
    char dir[512], bin[576], live[640];
    digit_module_manager_t *manager = NULL;
    digit_hotload_t *watcher = NULL;
    struct timespec before, after;
    long elapsed = -1;
    int ready = 0;
    if (!mkdtemp(root)) { CHECK(0, "timeout fixture root"); return; }
    snprintf(dir, sizeof(dir), "%s/sacrificial", root);
    snprintf(bin, sizeof(bin), "%s/bin", dir);
    snprintf(live, sizeof(live), "%s/sacrificial.so", bin);
    if (mkdir(dir, 0700) || mkdir(bin, 0700) ||
        !copy_fixture("build/tests/modules/sacrificial/sacrificial_hang.so", live))
        goto cleanup;
    manager = calloc(1, sizeof(*manager));
    watcher = calloc(1, sizeof(*watcher));
    if (!manager || !watcher) goto cleanup;
    digit_module_manager_init(manager, NULL);
    snprintf(manager->modules_path, sizeof(manager->modules_path), "%s", root);
    manager->registry.count = 1;
    strcpy(manager->registry.modules[0].descriptor.id, "sacrificial");
    manager->registry.modules[0].state = STNLABZ_MODULE_STATE_ACTIVE;
    digit_hotload_init(watcher, manager);
    ready = clock_gettime(CLOCK_MONOTONIC, &before) == 0;
    CHECK(ready, "timeout test clock available");
    if (!ready) goto cleanup;
    CHECK(digit_hotload_poll(watcher) == 0, "hung candidate rejected");
    if (clock_gettime(CLOCK_MONOTONIC, &after) == 0)
        elapsed = (after.tv_sec - before.tv_sec) * 1000L +
                  (after.tv_nsec - before.tv_nsec) / 1000000L;
    CHECK(elapsed >= 0 && elapsed < 6000L, "candidate timeout bounded to six seconds");
    CHECK(manager->registry.modules[0].state == STNLABZ_MODULE_STATE_ACTIVE,
          "hung qualification preserves incumbent ACTIVE");
    CHECK(watcher->count == 0, "hung candidate remains retryable");
cleanup:
    if (!ready) CHECK(0, "timeout fixture initialized");
    free(watcher);
    free(manager);
    unlink(live);
    rmdir(bin); rmdir(dir); rmdir(root);
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

    test_active_transition(0, 0, 0);
    test_active_transition(1, 0, 0);
    test_active_transition(0, 1, 0);
    test_active_transition(1, 0, 1);
    test_qualification_timeout();

    printf("\nHotload watcher tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
