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
    digit_corpus_record_t record, second, php, fetched, matches[4], listed[4];
    size_t count;

    unlink(path);
    memset(&record, 0, sizeof(record));
    snprintf(record.id, sizeof(record.id), "TEST-001");
    snprintf(record.category, sizeof(record.category), "RULE");
    snprintf(record.source, sizeof(record.source), "test");
    snprintf(record.text, sizeof(record.text), "Digit will enter safe mode on fatal Core error.");
    memset(&second, 0, sizeof(second));
    snprintf(second.id, sizeof(second.id), "TEST-002");
    snprintf(second.category, sizeof(second.category), "ENGINEERING");
    snprintf(second.source, sizeof(second.source), "test");
    snprintf(second.text, sizeof(second.text), "Optional telemetry module may be unavailable.");
    memset(&php, 0, sizeof(php));
    snprintf(php.id, sizeof(php.id), "TEST-003");
    snprintf(php.category, sizeof(php.category), "OPERATOR_LEARNED");
    snprintf(php.source, sizeof(php.source), "interface:learn");
    snprintf(php.text, sizeof(php.text), "PHP is an open-source server-side scripting language primarily designed for web development.");

    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "corpus") == 0, "module identity is corpus");
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 3 && descriptor->version_patch == 2, "internal version is 1.3.2");
    check(descriptor != NULL && descriptor->qualify(&qualification) == STNLABZ_MODULE_OK, "qualification executes");
    check(qualification.tests_executed >= STNLABZ_MODULE_MIN_TESTS && qualification.tests_passed == qualification.tests_executed && qualification.negative_test_executed && qualification.negative_test_passed, "qualification requirements pass");
    check(digit_corpus_validate(&record) && digit_corpus_append(path, &record) && digit_corpus_append(path, &second) && digit_corpus_append(path, &php), "valid records append");
    memset(&fetched, 0, sizeof(fetched));
    check(digit_corpus_get(path, "TEST-001", &fetched) && strcmp(fetched.text, record.text) == 0, "exact record retrieval works");
    memset(matches, 0, sizeof(matches)); count = digit_corpus_search(path, "safe mode", matches, 4);
    check(count >= 1 && strcmp(matches[0].id, "TEST-001") == 0, "exact phrase search remains deterministic");
    memset(matches, 0, sizeof(matches)); count = digit_corpus_search(path, "engineering", matches, 4);
    check(count >= 1 && strcmp(matches[0].id, "TEST-002") == 0, "category search works");
    memset(matches, 0, sizeof(matches)); count = digit_corpus_search(path, "what do you know about php web development", matches, 4);
    check(count >= 1 && strcmp(matches[0].id, "TEST-003") == 0, "natural question retrieves learned PHP knowledge");
    memset(listed, 0, sizeof(listed)); count = digit_corpus_list(path, listed, 4);
    check(count == 3 && strcmp(listed[0].id, "TEST-001") == 0 && strcmp(listed[1].id, "TEST-002") == 0 && strcmp(listed[2].id, "TEST-003") == 0, "deterministic corpus list returns complete ordered candidates");
    check(!digit_corpus_append(path, &record) && !digit_corpus_get(path, "MISSING", &fetched) && digit_corpus_search(path, "", matches, 4) == 0, "duplicate and invalid retrieval cases are rejected");

    /* [AI:GPT-6 | 2026-10-09] Regress bounded ABI record validation. */
    {
        digit_corpus_record_t malformed=record;
        memset(malformed.text,'A',sizeof(malformed.text));
        check(!digit_corpus_validate(&malformed) &&
              !digit_corpus_append(path,&malformed),
              "unterminated text is rejected before file append");
        malformed=record;
        memset(malformed.id,'A',sizeof(malformed.id));
        check(!digit_corpus_validate(&malformed),
              "unterminated record identity is rejected");
    }
    unlink(path);
    printf("\nCorpus module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
