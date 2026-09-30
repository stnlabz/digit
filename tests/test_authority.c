#include <stdio.h>
#include <string.h>

#include "authority.h"

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

static stnlabz_module_record_t qualified_record(const char *id)
{
    stnlabz_module_record_t record;
    memset(&record, 0, sizeof(record));
    record.descriptor = descriptor(id, 1, 0, 0);
    record.state = STNLABZ_MODULE_STATE_QUALIFIED;
    record.qualification.tests_executed = STNLABZ_MODULE_MIN_TESTS;
    record.qualification.tests_passed = STNLABZ_MODULE_MIN_TESTS;
    record.qualification.tests_failed = 0;
    record.qualification.negative_test_executed = 1;
    record.qualification.negative_test_passed = 1;
    record.activation_authorized = 1;
    return record;
}

int main(void)
{
    digit_qualification_inventory_t inventory;
    stnlabz_module_descriptor_t v100 = descriptor("test", 1, 0, 0);
    stnlabz_module_descriptor_t v101 = descriptor("test", 1, 0, 1);
    stnlabz_module_record_t candidate = qualified_record("test");
    stnlabz_module_record_t active = qualified_record("test");

    digit_qualification_init(&inventory);
    active.state = STNLABZ_MODULE_STATE_ACTIVE;

    CHECK(digit_authority_requires_qualification(&inventory, &v100),
          "unseen module version requires qualification");
    CHECK(digit_qualification_record(&inventory, &v100),
          "qualification evidence can be recorded");
    CHECK(!digit_authority_requires_qualification(&inventory, &v100),
          "known module version does not require retest");
    CHECK(digit_authority_requires_qualification(&inventory, &v101),
          "changed internal version requires qualification");
    CHECK(digit_authority_may_activate(&candidate) == DIGIT_AUTHORITY_ALLOW,
          "qualified authorized candidate may activate");

    candidate.qualification.tests_failed = 1;
    CHECK(digit_authority_may_activate(&candidate) == DIGIT_AUTHORITY_DENY,
          "failed required test denies activation");
    candidate = qualified_record("test");
    candidate.activation_authorized = 0;
    CHECK(digit_authority_may_activate(&candidate) == DIGIT_AUTHORITY_DENY,
          "missing activation authority denies activation");
    candidate = qualified_record("test");
    CHECK(digit_authority_may_replace(&active, &candidate) == DIGIT_AUTHORITY_ALLOW,
          "active module may be replaced only by qualified candidate");
    CHECK(digit_authority_may_dispatch(&active) == DIGIT_AUTHORITY_ALLOW,
          "active module may receive dispatch");
    CHECK(digit_authority_may_dispatch(&candidate) == DIGIT_AUTHORITY_DENY,
          "non-active module may not receive dispatch");

    printf("\nAuthority tests: %d executed, %d failed\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
