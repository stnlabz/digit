#include <stdio.h>
#include <string.h>

#include "qualification.h"

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
    digit_qualification_inventory_t inventory;
    stnlabz_module_descriptor_t v100 = descriptor("test", 1, 0, 0);
    stnlabz_module_descriptor_t v101 = descriptor("test", 1, 0, 1);
    stnlabz_module_descriptor_t other = descriptor("other", 1, 0, 0);

    digit_qualification_init(&inventory);

    CHECK(inventory.count == 0, "inventory starts empty");
    CHECK(!digit_qualification_contains(&inventory, &v100), "unqualified version is absent");
    CHECK(digit_qualification_record(&inventory, &v100), "qualified version can be recorded");
    CHECK(inventory.count == 1, "record increments inventory");
    CHECK(digit_qualification_contains(&inventory, &v100), "same internal version is qualified");
    CHECK(digit_qualification_record(&inventory, &v100), "duplicate qualification is accepted");
    CHECK(inventory.count == 1, "duplicate qualification is not duplicated");
    CHECK(!digit_qualification_contains(&inventory, &v101), "new internal version requires qualification");
    CHECK(!digit_qualification_contains(&inventory, &other), "different module identity is independent");
    CHECK(!digit_qualification_record(NULL, &v100), "record rejects null inventory");

    printf("\nQualification inventory tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
