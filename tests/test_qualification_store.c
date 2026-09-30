#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "qualification_store.h"

static int failures = 0;
static int tests = 0;

#define CHECK(condition, name) \
    do { \
        ++tests; \
        if (condition) { printf("PASS %02d - %s\n", tests, name); } \
        else { printf("FAIL %02d - %s\n", tests, name); ++failures; } \
    } while (0)

static stnlabz_module_descriptor_t descriptor(
    const char *id,
    unsigned int major,
    unsigned int minor,
    unsigned int patch
)
{
    stnlabz_module_descriptor_t value;
    memset(&value, 0, sizeof(value));
    snprintf(value.id, sizeof(value.id), "%s", id);
    value.version_major = major;
    value.version_minor = minor;
    value.version_patch = patch;
    return value;
}

int main(void)
{
    const char *path = "build/test_qualification.state";
    const char *missing = "build/test_qualification_missing.state";
    digit_qualification_inventory_t saved;
    digit_qualification_inventory_t loaded;
    stnlabz_module_descriptor_t one = descriptor("one", 1, 0, 0);
    stnlabz_module_descriptor_t two = descriptor("two", 2, 3, 4);
    FILE *bad;

    unlink(path);
    unlink(missing);
    digit_qualification_init(&saved);

    CHECK(digit_qualification_store_load(missing, &loaded),
          "missing store is valid empty state");
    CHECK(loaded.count == 0,
          "missing store loads zero qualifications");
    CHECK(digit_qualification_record(&saved, &one),
          "first qualification recorded");
    CHECK(digit_qualification_record(&saved, &two),
          "second qualification recorded");
    CHECK(digit_qualification_store_save(path, &saved),
          "qualification store saves");
    CHECK(digit_qualification_store_load(path, &loaded),
          "qualification store loads");
    CHECK(loaded.count == 2,
          "loaded qualification count matches");
    CHECK(digit_qualification_contains(&loaded, &one),
          "first qualification survives restart");
    CHECK(digit_qualification_contains(&loaded, &two),
          "second qualification survives restart");

    bad = fopen(path, "w");
    if (bad != NULL)
    {
        fputs("broken-state\n", bad);
        fclose(bad);
    }
    CHECK(!digit_qualification_store_load(path, &loaded),
          "malformed store is rejected");

    unlink(path);
    printf("\nQualification store tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
