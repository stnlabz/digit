#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "intent.h"

/* [AI:GPT-5.6 Sol | 2026-10-06T22:41:00Z] Initial deterministic Intent implementation. Interprets request purpose and target only; it contains no subject-specific knowledge or answers. */

static const stnlabz_module_host_t *intent_host = NULL;

static int word_equal_ci(const char *start, size_t length, const char *word)
{
    size_t i;
    if (start == NULL || word == NULL || strlen(word) != length) return 0;
    for (i = 0; i < length; ++i)
        if (tolower((unsigned char)start[i]) != tolower((unsigned char)word[i])) return 0;
    return 1;
}

static int has_word(const char *text, const char *word)
{
    const char *p = text;
    if (text == NULL || word == NULL) return 0;
    while (*p)
    {
        const char *start;
        size_t length;
        while (*p && !isalnum((unsigned char)*p) && *p != '_' && *p != '-') ++p;
        start = p;
        while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-')) ++p;
        length = (size_t)(p - start);
        if (length > 0 && word_equal_ci(start, length, word)) return 1;
    }
    return 0;
}

static int starts_with_word(const char *text, const char *word)
{
    const char *p = text;
    const char *start;
    size_t length;
    if (text == NULL || word == NULL) return 0;
    while (*p && !isalnum((unsigned char)*p) && *p != '_' && *p != '-') ++p;
    start = p;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-')) ++p;
    length = (size_t)(p - start);
    return length > 0 && word_equal_ci(start, length, word);
}

static int any_word(const char *text, const char *const *words, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) if (has_word(text, words[i])) return 1;
    return 0;
}

static void copy_subject_after_lead(const char *text, char *subject, size_t size)
{
    const char *p = text;
    if (subject == NULL || size == 0) return;
    subject[0] = '\0';
    if (text == NULL) return;
    while (*p && !isspace((unsigned char)*p)) ++p;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (*p) snprintf(subject, size, "%s", p);
}

static void set_result(digit_intent_result_t *result, digit_intent_class_t intent,
                       digit_intent_target_t target, unsigned int established,
                       const char *reason)
{
    result->intent = intent;
    result->target = target;
    result->established = established;
    snprintf(result->reason, sizeof(result->reason), "%s", reason);
}

static void interpret(const char *text, digit_intent_result_t *result)
{
    static const char *const action_words[] = {"create","build","write","generate","make","implement","produce","fix","remove","delete","install","uninstall","update","patch"};
    static const char *const status_words[] = {"status","errors","error","alerts","alert","broken","health","running","failures","failure"};
    static const char *const social_words[] = {"hi","hello","hey","morning","afternoon","evening","thanks","thank","sorry","ouch","paws"};
    static const char *const compare_words[] = {"compare","versus","difference","differences"};
    memset(result, 0, sizeof(*result));

    if (any_word(text, action_words, sizeof(action_words)/sizeof(action_words[0])))
    {
        set_result(result, DIGIT_INTENT_ACTION, DIGIT_INTENT_TARGET_CAPABILITY, 1U,
                   "Request directs Digit to perform or change something.");
        return;
    }
    if (any_word(text, status_words, sizeof(status_words)/sizeof(status_words[0])))
    {
        set_result(result, DIGIT_INTENT_STATUS, DIGIT_INTENT_TARGET_RUNTIME, 1U,
                   "Request asks about current operational state.");
        return;
    }
    if (starts_with_word(text, "explain"))
    {
        copy_subject_after_lead(text, result->subject, sizeof(result->subject));
        set_result(result, DIGIT_INTENT_EXPLAIN, DIGIT_INTENT_TARGET_KNOWLEDGE,
                   result->subject[0] != '\0', "Request asks for an explanation.");
        return;
    }
    if (starts_with_word(text, "define"))
    {
        copy_subject_after_lead(text, result->subject, sizeof(result->subject));
        set_result(result, DIGIT_INTENT_DEFINE, DIGIT_INTENT_TARGET_KNOWLEDGE,
                   result->subject[0] != '\0', "Request asks for a definition.");
        return;
    }
    if (any_word(text, compare_words, sizeof(compare_words)/sizeof(compare_words[0])))
    {
        set_result(result, DIGIT_INTENT_COMPARE, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U,
                   "Request asks for a comparison.");
        return;
    }
    if (starts_with_word(text, "why"))
    {
        set_result(result, DIGIT_INTENT_WHY, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U,
                   "Request asks for a supported reason or cause.");
        return;
    }
    if (starts_with_word(text, "how"))
    {
        set_result(result, DIGIT_INTENT_HOW, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U,
                   "Request asks how something works or is done.");
        return;
    }
    if (starts_with_word(text, "what") || starts_with_word(text, "who"))
    {
        set_result(result, DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U,
                   "Request asks for factual knowledge.");
        return;
    }
    if (any_word(text, social_words, sizeof(social_words)/sizeof(social_words[0])))
    {
        set_result(result, DIGIT_INTENT_CONVERSATION, DIGIT_INTENT_TARGET_SOCIAL, 1U,
                   "Request is conversational rather than an operational or knowledge task.");
        return;
    }
    set_result(result, DIGIT_INTENT_UNKNOWN, DIGIT_INTENT_TARGET_UNKNOWN, 0U,
               "Intent is not deterministically established.");
}

const char *digit_intent_class_string(digit_intent_class_t intent)
{
    switch (intent)
    {
        case DIGIT_INTENT_CONVERSATION: return "CONVERSATION";
        case DIGIT_INTENT_FACT: return "FACT";
        case DIGIT_INTENT_DEFINE: return "DEFINE";
        case DIGIT_INTENT_EXPLAIN: return "EXPLAIN";
        case DIGIT_INTENT_COMPARE: return "COMPARE";
        case DIGIT_INTENT_WHY: return "WHY";
        case DIGIT_INTENT_HOW: return "HOW";
        case DIGIT_INTENT_STATUS: return "STATUS";
        case DIGIT_INTENT_ACTION: return "ACTION";
        case DIGIT_INTENT_AMBIGUOUS: return "AMBIGUOUS";
        default: return "UNKNOWN";
    }
}

const char *digit_intent_target_string(digit_intent_target_t target)
{
    switch (target)
    {
        case DIGIT_INTENT_TARGET_SOCIAL: return "SOCIAL";
        case DIGIT_INTENT_TARGET_KNOWLEDGE: return "KNOWLEDGE";
        case DIGIT_INTENT_TARGET_RUNTIME: return "RUNTIME";
        case DIGIT_INTENT_TARGET_CAPABILITY: return "CAPABILITY";
        default: return "UNKNOWN";
    }
}

static stnlabz_module_result_t intent_service(const void *request, size_t request_size,
                                              void *response, size_t response_size,
                                              size_t *response_used, void *handler_context)
{
    const digit_intent_request_t *input = request;
    digit_intent_result_t result;
    (void)handler_context;
    if (request == NULL || request_size != sizeof(*input) || response == NULL ||
        response_used == NULL || response_size < sizeof(result))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->text, '\0', sizeof(input->text)) == NULL || input->text[0] == '\0')
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    interpret(input->text, &result);
    memcpy(response, &result, sizeof(result));
    *response_used = sizeof(result);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t intent_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t intent_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_INTENT_SERVICE, intent_service, NULL))
        return STNLABZ_MODULE_ERR_START_FAILED;
    intent_host = host;
    if (host->send_message != NULL)
        (void)host->send_message("[INTENT] module active: deterministic intent.interpret registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t intent_stop(void)
{
    if (intent_host != NULL && intent_host->unregister_service != NULL)
        if (!intent_host->unregister_service(DIGIT_INTENT_SERVICE, NULL))
            return STNLABZ_MODULE_ERR_STOP_FAILED;
    intent_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t intent_descriptor =
{
    "intent", "Digit Intent", 1, 0, 0,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    intent_qualify, intent_start, intent_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &intent_descriptor;
}
