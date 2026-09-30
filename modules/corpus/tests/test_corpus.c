#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "corpus.h"

static unsigned int executed = 0;
static unsigned int failed = 0;

static void check(int condition, const char *name)
{
    ++executed;
    if (condition) printf("PASS %02u - %s\n", executed, name);
    else { ++failed; printf("FAIL %02u - %s\n", executed, name); }
}

int main(void)
{
    const char *path = "/tmp/digit-corpus-test.tsv";
    const stnlabz_module_descriptor_t *descriptor = stnlabz_module_get_descriptor();
    stnlabz_module_qualification_result_t qualification;
    digit_corpus_record_t record;

    unlink(path);
    memset(&record, 0, sizeof(record));
    snprintf(record.id, sizeof(record.id), "TEST-001");
    snprintf(record.category, sizeof(record.category), "ENGINEERING");
    snprintf(record.source, sizeof(record.source), "test");
    snprintf(record.text, sizeof(record.text), "Digit corpus test record.");

    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "corpus") == 0, "module identity is corpus");
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 0 && descriptor->version_patch == 0, "internal version is 1.0.0");
    check(descriptor != NULL && descriptor->qualify(&qualification) == STNLABZ_MODULE_OK, "qualification executes");
    check(qualification.tests_executed >= STNLABZ_MODULE_MIN_TESTS && qualification.tests_passed == qualification.tests_executed, "required qualification tests pass");
    check(qualification.negative_test_executed && qualification.negative_test_passed, "negative validation passes");
    check(digit_corpus_validate(&record), "valid corpus record accepted");
    check(digit_corpus_append(path, &record), "record appends to corpus");
    check(digit_corpus_contains(path, record.id), "stored record is discoverable");
    check(!digit_corpus_append(path, &record) && !digit_corpus_validate(NULL), "duplicate and invalid records are rejected");

    unlink(path);
    printf("\nCorpus module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
