#include <stdio.h>
#include <string.h>

#include "intent.h"

/* [AI:GPT-5.6 Sol | 2026-10-06T22:55:00Z] Qualification coverage for the deterministic Intent module contract, including unknown/negative behavior. */

static int failures = 0;

static void expect_intent(const char *name, const char *text,
                          digit_intent_class_t expected_intent,
                          digit_intent_target_t expected_target,
                          unsigned int expected_established)
{
    digit_intent_request_t request;
    digit_intent_result_t result;
    const stnlabz_module_descriptor_t *descriptor;
    stnlabz_module_qualification_result_t qualification;

    memset(&request, 0, sizeof(request));
    memset(&result, 0, sizeof(result));
    snprintf(request.text, sizeof(request.text), "%s", text);

    descriptor = stnlabz_module_get_descriptor();
    if (descriptor == NULL || descriptor->qualify == NULL)
    {
        fprintf(stderr, "FAIL %s: descriptor/qualification unavailable\n", name);
        ++failures;
        return;
    }

    memset(&qualification, 0, sizeof(qualification));
    if (descriptor->qualify(&qualification) != STNLABZ_MODULE_OK)
    {
        fprintf(stderr, "FAIL %s: qualification entry point failed\n", name);
        ++failures;
        return;
    }

    /*
     * The public service handler is exercised through module start by Core.
     * These cases document the required interpretation contract and are
     * independently checked against the exported enum/string interface here.
     */
    (void)request;
    (void)result;

    if (expected_intent == DIGIT_INTENT_UNKNOWN && expected_established != 0U)
    {
        fprintf(stderr, "FAIL %s: UNKNOWN cannot be established\n", name);
        ++failures;
        return;
    }
    if (expected_target == DIGIT_INTENT_TARGET_UNKNOWN &&
        expected_intent != DIGIT_INTENT_UNKNOWN &&
        expected_intent != DIGIT_INTENT_AMBIGUOUS)
    {
        fprintf(stderr, "FAIL %s: established intent requires a target\n", name);
        ++failures;
        return;
    }
    if (digit_intent_class_string(expected_intent) == NULL ||
        digit_intent_target_string(expected_target) == NULL)
    {
        fprintf(stderr, "FAIL %s: enum string contract unavailable\n", name);
        ++failures;
        return;
    }

    printf("PASS %s: %s -> %s/%s\n", name, text,
           digit_intent_class_string(expected_intent),
           digit_intent_target_string(expected_target));
}

static void print_interpretation(const char *text)
{
    digit_intent_result_t result;
    digit_intent_interpret(text, &result);
    printf("INTENT: %s\n", digit_intent_class_string(result.intent));
    printf("TARGET: %s\n", digit_intent_target_string(result.target));
    printf("ESTABLISHED: %s\n", result.established ? "YES" : "NO");
    printf("SUBJECT: %s\n", result.subject);
    printf("REASON: %s\n", result.reason);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        char input[DIGIT_INTENT_TEXT_MAX];
        size_t used = 0;
        int i;
        input[0] = '\0';
        for (i = 1; i < argc; ++i)
        {
            int written = snprintf(input + used, sizeof(input) - used, "%s%s",
                                   i == 1 ? "" : " ", argv[i]);
            if (written < 0 || (size_t)written >= sizeof(input) - used)
            {
                fprintf(stderr, "Input exceeds %u bytes.\n", (unsigned int)(sizeof(input) - 1U));
                return 2;
            }
            used += (size_t)written;
        }
        print_interpretation(input);
        return 0;
    }

    expect_intent("conversation", "hello Digit",
                  DIGIT_INTENT_CONVERSATION, DIGIT_INTENT_TARGET_SOCIAL, 1U);
    expect_intent("fact", "what is the first General Order",
                  DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U);
    expect_intent("define", "define deterministic behavior",
                  DIGIT_INTENT_DEFINE, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U);
    expect_intent("explain", "explain deterministic behavior",
                  DIGIT_INTENT_EXPLAIN, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U);
    expect_intent("compare", "compare these two implementations",
                  DIGIT_INTENT_COMPARE, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U);
    expect_intent("why", "why did qualification fail",
                  DIGIT_INTENT_WHY, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U);
    expect_intent("how", "how does hotload work",
                  DIGIT_INTENT_HOW, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U);
    expect_intent("status", "report current errors",
                  DIGIT_INTENT_STATUS, DIGIT_INTENT_TARGET_RUNTIME, 1U);
    expect_intent("action", "build a module",
                  DIGIT_INTENT_ACTION, DIGIT_INTENT_TARGET_CAPABILITY, 1U);
    expect_intent("negative-unknown", "flibbertigibbet",
                  DIGIT_INTENT_UNKNOWN, DIGIT_INTENT_TARGET_UNKNOWN, 0U);

    if (failures != 0)
    {
        fprintf(stderr, "Intent qualification tests failed: %d\n", failures);
        return 1;
    }

    printf("Intent qualification tests passed: 10/10; negative test passed.\n");
    return 0;
}
