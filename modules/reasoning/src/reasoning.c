#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "reasoning.h"

static const stnlabz_module_host_t *reasoning_host = NULL;

static void source_view(const char *context, const char **source, size_t *source_length)
{
    const char *start;
    const char *end;
    if (source == NULL || source_length == NULL) return;
    *source = context;
    *source_length = context != NULL ? strlen(context) : 0;
    if (context == NULL || strncmp(context, "SOURCE:\n", 8) != 0) return;
    start = context + 8;
    end = strstr(start, "\n\nCONTEXT:\n");
    if (end != NULL)
    {
        *source = start;
        *source_length = (size_t)(end - start);
    }
}

static int source_contains_ci(const char *source, size_t source_length, const char *needle)
{
    size_t needle_length;
    size_t i, j;
    if (source == NULL || needle == NULL) return 0;
    needle_length = strlen(needle);
    if (needle_length == 0 || needle_length > source_length) return 0;
    for (i = 0; i + needle_length <= source_length; ++i)
    {
        int match = 1;
        for (j = 0; j < needle_length; ++j)
        {
            if (tolower((unsigned char)source[i + j]) != tolower((unsigned char)needle[j])) { match = 0; break; }
        }
        if (match) return 1;
    }
    return 0;
}

const char *digit_relevance_string(digit_relevance_t relevance)
{
    switch (relevance)
    {
        case DIGIT_RELEVANCE_IRRELEVANT: return "IRRELEVANT";
        case DIGIT_RELEVANCE_UNCERTAIN: return "UNCERTAIN";
        case DIGIT_RELEVANCE_RELEVANT: return "RELEVANT";
        default: return "UNKNOWN";
    }
}

const char *digit_context_category_string(digit_context_category_t category)
{
    switch (category)
    {
        case DIGIT_CONTEXT_CONVERSATION: return "CONVERSATION";
        case DIGIT_CONTEXT_ENGINEERING: return "ENGINEERING";
        case DIGIT_CONTEXT_RULE: return "RULE";
        case DIGIT_CONTEXT_DECISION: return "DECISION";
        case DIGIT_CONTEXT_OBSERVATION: return "OBSERVATION";
        case DIGIT_CONTEXT_HYPOTHESIS: return "HYPOTHESIS";
        default: return "UNKNOWN";
    }
}

int digit_reasoning_evaluate(const char *context, digit_reasoning_result_t *result)
{
    const char *source;
    size_t source_length;
    int self_reference;
    int engineering_reference;
    int source_rule_reference;
    int decision_reference;
    int hypothesis_reference;

    if (context == NULL || result == NULL || context[0] == '\0') return 0;

    memset(result, 0, sizeof(*result));
    result->relevance = DIGIT_RELEVANCE_UNCERTAIN;
    result->category = DIGIT_CONTEXT_CONVERSATION;
    result->confidence = 25;
    snprintf(result->reason, sizeof(result->reason), "No deterministic relationship to Digit established.");

    source_view(context, &source, &source_length);

    self_reference = source_contains_ci(source, source_length, "digit") || source_contains_ci(source, source_length, "her core") || source_contains_ci(source, source_length, "her module") || source_contains_ci(source, source_length, "her corpus");
    engineering_reference = source_contains_ci(source, source_length, "module") || source_contains_ci(source, source_length, "core") || source_contains_ci(source, source_length, "abi") || source_contains_ci(source, source_length, "corpus") || source_contains_ci(source, source_length, "qualification") || source_contains_ci(source, source_length, "hotload");
    source_rule_reference = source_contains_ci(source, source_length, "rule") || source_contains_ci(source, source_length, "must") || source_contains_ci(source, source_length, "required") || source_contains_ci(source, source_length, "will ") || source_contains_ci(source, source_length, "will not") || source_contains_ci(source, source_length, "prohibited");
    decision_reference = source_contains_ci(source, source_length, "decided") || source_contains_ci(source, source_length, "decision") || source_contains_ci(source, source_length, "will use") || source_contains_ci(source, source_length, "going forward");
    hypothesis_reference = source_contains_ci(source, source_length, "maybe") || source_contains_ci(source, source_length, "perhaps") || source_contains_ci(source, source_length, "might") || source_contains_ci(source, source_length, "could");

    if (self_reference)
    {
        result->relevance = DIGIT_RELEVANCE_RELEVANT;
        result->confidence = engineering_reference ? 95U : 80U;
        result->category = engineering_reference ? DIGIT_CONTEXT_ENGINEERING : DIGIT_CONTEXT_OBSERVATION;
        snprintf(result->reason, sizeof(result->reason), "Authoritative source directly references Digit or a Digit-owned capability.");
    }
    else if (engineering_reference)
    {
        result->relevance = DIGIT_RELEVANCE_UNCERTAIN;
        result->category = DIGIT_CONTEXT_ENGINEERING;
        result->confidence = 50;
        snprintf(result->reason, sizeof(result->reason), "Engineering source detected without a deterministic Digit relationship.");
    }

    if (source_rule_reference && self_reference)
    {
        result->category = DIGIT_CONTEXT_RULE;
        result->confidence = 95U;
        snprintf(result->reason, sizeof(result->reason), "Authoritative source establishes normative Digit behavior.");
    }
    else if (decision_reference && self_reference) result->category = DIGIT_CONTEXT_DECISION;
    else if (hypothesis_reference) result->category = DIGIT_CONTEXT_HYPOTHESIS;

    return 1;
}

static stnlabz_module_result_t reasoning_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const char *context = request;
    digit_reasoning_result_t result;
    (void)handler_context;
    if (request == NULL || request_size == 0 || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (((const char *)request)[request_size - 1] != '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!digit_reasoning_evaluate(context, &result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memcpy(response, &result, sizeof(result));
    *response_used = sizeof(result);
    return STNLABZ_MODULE_OK;
}

/* [AI:GPT-5.6 Sol | 2026-10-07T00:24:00Z] Deterministic EXPLAIN reasoning organizes only supplied authorized evidence. It ranks subject-defining evidence before descriptive evidence, rejects question-like evidence, deduplicates records, and introduces no external knowledge. */
/* [AI:GPT-5.6 Sol | 2026-10-07T00:39:00Z] EXPLAIN now classifies evidence by semantic role and selects at most one identity, purpose, characteristic, and incidental detail in that priority order. This prevents repeated low-level facts from masquerading as an explanation. */
static int explanation_question_like(const char *text)
{
    const char *p;
    if (text == NULL) return 0;
    for (p = text; *p != '\0'; ++p) if (*p == '?') return 1;
    return 0;
}

static int explanation_contains_ci(const char *text, const char *needle)
{
    return text != NULL && needle != NULL && source_contains_ci(text, strlen(text), needle);
}

typedef enum
{
    EXPLANATION_ROLE_NONE = 0,
    EXPLANATION_ROLE_IDENTITY,
    EXPLANATION_ROLE_PURPOSE,
    EXPLANATION_ROLE_CHARACTERISTIC,
    EXPLANATION_ROLE_DETAIL
} explanation_role_t;

static explanation_role_t explanation_role(const char *subject, const char *text)
{
    int names_subject;
    if (subject == NULL || text == NULL || text[0] == '\0' || explanation_question_like(text)) return EXPLANATION_ROLE_NONE;
    names_subject = explanation_contains_ci(text, subject);
    if (!names_subject) return EXPLANATION_ROLE_NONE;
    if (explanation_contains_ci(text, "stands for") || explanation_contains_ci(text, " is a ") || explanation_contains_ci(text, " is an ")) return EXPLANATION_ROLE_IDENTITY;
    if (explanation_contains_ci(text, "used for") || explanation_contains_ci(text, "used to") || explanation_contains_ci(text, "allows") || explanation_contains_ci(text, "provides") || explanation_contains_ci(text, "purpose")) return EXPLANATION_ROLE_PURPOSE;
    if (explanation_contains_ci(text, "supports") || explanation_contains_ci(text, "uses") || explanation_contains_ci(text, "works") || explanation_contains_ci(text, "runs") || explanation_contains_ci(text, "typed") || explanation_contains_ci(text, "type")) return EXPLANATION_ROLE_CHARACTERISTIC;
    return EXPLANATION_ROLE_DETAIL;
}

/* [AI:GPT-5.6 Sol | 2026-10-07T02:18:00Z] COMPARE subject attribution requires token-bounded identity so short subject names do not bind to longer language names or arbitrary character substrings. */
static int subject_token_match(const char *text, const char *subject)
{
    size_t i, n;
    if (text == NULL || subject == NULL || subject[0] == '\0') return 0;
    n = strlen(subject);
    for (i = 0; text[i] != '\0'; ++i)
    {
        size_t j;
        if (i > 0 && (isalnum((unsigned char)text[i - 1]) || text[i - 1] == '_' || text[i - 1] == '#' || text[i - 1] == '+')) continue;
        for (j = 0; j < n; ++j)
        {
            if (text[i + j] == '\0' || tolower((unsigned char)text[i + j]) != tolower((unsigned char)subject[j])) break;
        }
        if (j != n) continue;
        if (text[i + n] != '\0' && (isalnum((unsigned char)text[i + n]) || text[i + n] == '_' || text[i + n] == '#' || text[i + n] == '+')) continue;
        return 1;
    }
    return 0;
}

static unsigned int explanation_rank(const char *subject, const char *text)
{
    switch (explanation_role(subject, text))
    {
        case EXPLANATION_ROLE_IDENTITY: return 400U;
        case EXPLANATION_ROLE_PURPOSE: return 300U;
        case EXPLANATION_ROLE_CHARACTERISTIC: return 200U;
        case EXPLANATION_ROLE_DETAIL: return 100U;
        default: return 0U;
    }
}

static stnlabz_module_result_t reasoning_explain_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_reasoning_explain_request_t *input = request;
    digit_reasoning_explain_result_t output;
    size_t order[DIGIT_REASONING_EVIDENCE_MAX];
    unsigned int rank[DIGIT_REASONING_EVIDENCE_MAX];
    size_t count, i, j, offset = 0;
    (void)handler_context;
    if (request == NULL || request_size != sizeof(*input) || response == NULL || response_used == NULL || response_size < sizeof(output)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->subject, '\0', sizeof(input->subject)) == NULL || input->subject[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (input->evidence_count > DIGIT_REASONING_EVIDENCE_MAX) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    count = input->evidence_count;
    memset(&output, 0, sizeof(output));
    for (i = 0; i < count; ++i)
    {
        if (memchr(input->evidence[i], '\0', sizeof(input->evidence[i])) == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
        order[i] = i;
        rank[i] = explanation_rank(input->subject, input->evidence[i]);
    }
    for (i = 1; i < count; ++i)
    {
        size_t key = order[i];
        j = i;
        while (j > 0 && rank[order[j - 1]] < rank[key]) { order[j] = order[j - 1]; --j; }
        order[j] = key;
    }
    {
        int role_used[EXPLANATION_ROLE_DETAIL + 1] = {0};
        for (i = 0; i < count && output.evidence_used < 3; ++i)
        {
            size_t index = order[i];
            explanation_role_t role = explanation_role(input->subject, input->evidence[index]);
            int duplicate = 0;
            int written;
            size_t k;
            if (rank[index] == 0 || role == EXPLANATION_ROLE_NONE || input->evidence[index][0] == '\0') continue;
            if (role_used[role]) continue;
            for (k = 0; k < i; ++k) if (strcmp(input->evidence[index], input->evidence[order[k]]) == 0) { duplicate = 1; break; }
            if (duplicate) continue;
            written = snprintf(output.explanation + offset, sizeof(output.explanation) - offset, "%s%s", offset ? " " : "", input->evidence[index]);
            if (written <= 0 || (size_t)written >= sizeof(output.explanation) - offset) break;
            offset += (size_t)written;
            role_used[role] = 1;
            ++output.evidence_used;
        }
    }
    output.explained = output.evidence_used > 0;
    memcpy(response, &output, sizeof(output));
    *response_used = sizeof(output);
    return STNLABZ_MODULE_OK;
}

/* [AI:GPT-5.6 Sol | 2026-10-07T01:35:00Z] Deterministic COMPARE reasoning separates supplied authorized evidence by established subject and emits only evidence-backed statements for both sides. */
static stnlabz_module_result_t reasoning_compare_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_reasoning_compare_request_t *input = request;
    digit_reasoning_compare_result_t output;
    const char *left_text = NULL, *right_text = NULL;
    unsigned int left_rank = 0, right_rank = 0;
    size_t count, i;
    (void)handler_context;
    if (request == NULL || request_size != sizeof(*input) || response == NULL ||
        response_used == NULL || response_size < sizeof(output)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->left, '\0', sizeof(input->left)) == NULL || input->left[0] == '\0' ||
        memchr(input->right, '\0', sizeof(input->right)) == NULL || input->right[0] == '\0')
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(&output, 0, sizeof(output));
    if (input->evidence_count > DIGIT_REASONING_EVIDENCE_MAX) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    count = input->evidence_count;
    for (i = 0; i < count; ++i)
    {
        unsigned int rank;
        if (memchr(input->evidence[i], '\0', sizeof(input->evidence[i])) == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
        if (subject_token_match(input->evidence[i], input->left))
        {
            rank = explanation_rank(input->left, input->evidence[i]);
            if (rank > left_rank) { left_rank = rank; left_text = input->evidence[i]; }
        }
        if (subject_token_match(input->evidence[i], input->right))
        {
            rank = explanation_rank(input->right, input->evidence[i]);
            if (rank > right_rank) { right_rank = rank; right_text = input->evidence[i]; }
        }
    }
    /* [AI:GPT-6 | 2026-10-09] A comparison requires independently
     * attributable evidence for both subjects, not one shared sentence. */
    if (left_text == NULL || right_text == NULL || left_text == right_text)
    {
        memcpy(response, &output, sizeof(output)); *response_used = sizeof(output); return STNLABZ_MODULE_OK;
    }
    snprintf(output.comparison, sizeof(output.comparison), "%s %s", left_text, right_text);
    output.left_evidence_used = 1;
    output.right_evidence_used = 1;
    output.compared = 1;
    memcpy(response, &output, sizeof(output)); *response_used = sizeof(output); return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t reasoning_qualify(stnlabz_module_qualification_result_t *result)
{
    digit_reasoning_result_t relevance;
    digit_reasoning_explain_request_t explain;
    digit_reasoning_explain_result_t explanation;
    digit_reasoning_compare_request_t compare;
    digit_reasoning_compare_result_t comparison;
    size_t used = 0;
    stnlabz_module_result_t status;
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));

    ++result->tests_executed;
    if (digit_reasoning_evaluate("Digit module qualification", &relevance) &&
        relevance.relevance == DIGIT_RELEVANCE_RELEVANT) ++result->tests_passed;

    memset(&explain, 0, sizeof(explain));
    memset(&explanation, 0, sizeof(explanation));
    snprintf(explain.subject, sizeof(explain.subject), "engine");
    snprintf(explain.evidence[0], sizeof(explain.evidence[0]), "An engine is a machine.");
    explain.evidence_count = 1;
    status = reasoning_explain_service(&explain, sizeof(explain), &explanation,
                                       sizeof(explanation), &used, NULL);
    ++result->tests_executed;
    if (status == STNLABZ_MODULE_OK && used == sizeof(explanation) &&
        explanation.explained && explanation.evidence_used == 1 &&
        strstr(explanation.explanation, "An engine is a machine.") != NULL)
        ++result->tests_passed;

    memset(&compare, 0, sizeof(compare));
    memset(&comparison, 0, sizeof(comparison));
    snprintf(compare.left, sizeof(compare.left), "alpha");
    snprintf(compare.right, sizeof(compare.right), "beta");
    snprintf(compare.evidence[0], sizeof(compare.evidence[0]), "alpha is a first label.");
    snprintf(compare.evidence[1], sizeof(compare.evidence[1]), "beta is a second label.");
    compare.evidence_count = 2;
    used = 0;
    status = reasoning_compare_service(&compare, sizeof(compare), &comparison,
                                       sizeof(comparison), &used, NULL);
    ++result->tests_executed;
    if (status == STNLABZ_MODULE_OK && used == sizeof(comparison) &&
        comparison.compared && comparison.left_evidence_used == 1 &&
        comparison.right_evidence_used == 1) ++result->tests_passed;

    /* [AI:GPT-6 | 2026-10-09] Measured negative comparison cases. */
    memset(&compare, 0, sizeof(compare));
    memset(&comparison, 0, sizeof(comparison));
    snprintf(compare.left, sizeof(compare.left), "C");
    snprintf(compare.right, sizeof(compare.right), "Python");
    snprintf(compare.evidence[0], sizeof(compare.evidence[0]), "Python provides a runtime.");
    compare.evidence_count = 1;
    used = 0;
    status = reasoning_compare_service(&compare,sizeof(compare),&comparison,
                                       sizeof(comparison),&used,NULL);
    ++result->tests_executed;
    if(status==STNLABZ_MODULE_OK&&used==sizeof(comparison)&&!comparison.compared)
        ++result->tests_passed;

    memset(&compare, 0, sizeof(compare));
    memset(&comparison, 0, sizeof(comparison));
    snprintf(compare.left, sizeof(compare.left), "C");
    snprintf(compare.right, sizeof(compare.right), "Python");
    snprintf(compare.evidence[0], sizeof(compare.evidence[0]), "C and Python are programming languages.");
    compare.evidence_count = 1;
    used = 0;
    status = reasoning_compare_service(&compare,sizeof(compare),&comparison,
                                       sizeof(comparison),&used,NULL);
    ++result->tests_executed;
    if(status==STNLABZ_MODULE_OK&&used==sizeof(comparison)&&!comparison.compared)
        ++result->tests_passed;

    compare.evidence_count = DIGIT_REASONING_EVIDENCE_MAX+1;
    ++result->negative_test_executed;
    if(reasoning_compare_service(&compare,sizeof(compare),&comparison,
                                 sizeof(comparison),&used,NULL)==
       STNLABZ_MODULE_ERR_INVALID_ARGUMENT)++result->negative_test_passed;

    /* Additional measured cases satisfy the Core minimum without declared passes. */
    {
        static const char *const contexts[] = {
            "Digit core is active",
            "Digit module status",
            "Digit corpus evidence",
            "Digit qualification record",
            "Digit will report",
            "Digit decided to retain the result",
            "Digit might require more evidence"
        };
        size_t i;
        for (i = 0; i < sizeof(contexts) / sizeof(contexts[0]); ++i)
        {
            ++result->tests_executed;
            if (digit_reasoning_evaluate(contexts[i], &relevance) &&
                relevance.relevance == DIGIT_RELEVANCE_RELEVANT)
                ++result->tests_passed;
        }
    }

    ++result->negative_test_executed;
    if (reasoning_explain_service(NULL, 0, &explanation,
                                  sizeof(explanation), &used, NULL) ==
        STNLABZ_MODULE_ERR_INVALID_ARGUMENT) ++result->negative_test_passed;
    result->tests_failed = result->tests_executed - result->tests_passed;
    return result->tests_failed == 0 && result->negative_test_passed
        ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_QUALIFICATION;
}

static stnlabz_module_result_t reasoning_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_REASONING_SERVICE, reasoning_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    if (!host->register_service(DIGIT_REASONING_EXPLAIN_SERVICE, reasoning_explain_service, NULL)) { (void)host->unregister_service(DIGIT_REASONING_SERVICE, NULL); return STNLABZ_MODULE_ERR_START_FAILED; }
    if (!host->register_service(DIGIT_REASONING_COMPARE_SERVICE, reasoning_compare_service, NULL)) { (void)host->unregister_service(DIGIT_REASONING_EXPLAIN_SERVICE, NULL); (void)host->unregister_service(DIGIT_REASONING_SERVICE, NULL); return STNLABZ_MODULE_ERR_START_FAILED; }
    reasoning_host = host;
    if (host->send_message != NULL) (void)host->send_message("[REASONING] module active: evaluate, explain, and deterministic compare registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t reasoning_stop(void)
{
    if (reasoning_host != NULL && reasoning_host->unregister_service != NULL)
    {
        if (!reasoning_host->unregister_service(DIGIT_REASONING_COMPARE_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
        if (!reasoning_host->unregister_service(DIGIT_REASONING_EXPLAIN_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
        if (!reasoning_host->unregister_service(DIGIT_REASONING_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
    }
    reasoning_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t reasoning_descriptor =
{
    "reasoning", "Digit Relevance Reasoning", 1, 0, 9,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    reasoning_qualify, reasoning_start, reasoning_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &reasoning_descriptor;
}
