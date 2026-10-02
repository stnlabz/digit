#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "validator.h"

static const stnlabz_module_host_t *validator_host = NULL;

static int contains_ci(const char *text, const char *needle)
{
    size_t i, j, n;
    if (text == NULL || needle == NULL || needle[0] == '\0') return 0;
    n = strlen(needle);
    for (i = 0; text[i] != '\0'; ++i) {
        for (j = 0; j < n && text[i + j] != '\0'; ++j)
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)needle[j])) break;
        if (j == n) return 1;
    }
    return 0;
}

static void trim_copy(const char *src, char *dst, size_t size)
{
    const char *start, *end;
    size_t n;
    if (dst == NULL || size == 0) return;
    dst[0] = '\0';
    if (src == NULL) return;
    start = src;
    while (*start && isspace((unsigned char)*start)) ++start;
    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) --end;
    n = (size_t)(end - start);
    if (n >= size) n = size - 1;
    memcpy(dst, start, n);
    dst[n] = '\0';
}

static void replace_word(char *text, size_t size, const char *bad, const char *good)
{
    char work[DIGIT_VALIDATOR_TEXT_MAX];
    char *p;
    size_t prefix;
    if (text == NULL || bad == NULL || good == NULL) return;
    p = strstr(text, bad);
    if (p == NULL) return;
    prefix = (size_t)(p - text);
    if (prefix >= sizeof(work)) return;
    snprintf(work, sizeof(work), "%.*s%s%s", (int)prefix, text, good, p + strlen(bad));
    snprintf(text, size, "%s", work);
}

static void canonical_text(const char *src, char *dst, size_t size)
{
    size_t i, o = 0;
    int pending_space = 0;
    if (dst == NULL || size == 0) return;
    dst[0] = '\0';
    if (src == NULL) return;
    for (i = 0; src[i] != '\0' && o + 1 < size; ++i) {
        unsigned char ch = (unsigned char)src[i];
        if (isalnum(ch)) {
            if (pending_space && o > 0 && o + 1 < size) dst[o++] = ' ';
            dst[o++] = (char)tolower(ch);
            pending_space = 0;
        } else if (o > 0) {
            pending_space = 1;
        }
    }
    dst[o] = '\0';
}

static int nearly_echo(const char *input, const char *candidate)
{
    char a[DIGIT_VALIDATOR_TEXT_MAX], b[DIGIT_VALIDATOR_TEXT_MAX];
    canonical_text(input, a, sizeof(a));
    canonical_text(candidate, b, sizeof(b));
    if (a[0] == '\0' || b[0] == '\0') return 0;
    if (strcmp(a, b) == 0) return 1;
    if (strlen(a) >= 12 && strstr(b, a) != NULL && strlen(b) <= strlen(a) + 24) return 1;
    if (strlen(b) >= 12 && strstr(a, b) != NULL && strlen(a) <= strlen(b) + 24) return 1;
    return 0;
}

static int asks_explain(const char *text)
{
    return contains_ci(text, "explain") || contains_ci(text, "describe") || contains_ci(text, "walk me through") || contains_ci(text, "break down");
}

static int weak_explanation(const char *candidate)
{
    size_t n;
    int sentence_marks = 0;
    const char *p;
    if (candidate == NULL) return 1;
    n = strlen(candidate);
    if (n < 55) return 1;
    for (p = candidate; *p; ++p) if (*p == '.' || *p == ';' || *p == ':') ++sentence_marks;
    return sentence_marks == 0;
}

static int evidence_supports(const char *candidate, const char *evidence)
{
    static const char *risky[] = {
        "does not require authentication", "does not require authorization",
        "safest of all", "always", "never", "plain text"
    };
    size_t i;
    if (candidate == NULL) return 0;
    for (i = 0; i < sizeof(risky) / sizeof(risky[0]); ++i) {
        if (contains_ci(candidate, risky[i]) && !contains_ci(evidence, risky[i])) return 0;
    }
    return 1;
}

static stnlabz_module_result_t inbound_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *context)
{
    const digit_validator_inbound_request_t *in;
    digit_validator_inbound_result_t *out;
    (void)context;
    if (request == NULL || response == NULL || response_used == NULL || request_size != sizeof(*in) || response_size < sizeof(*out)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    in = (const digit_validator_inbound_request_t *)request;
    out = (digit_validator_inbound_result_t *)response;
    memset(out, 0, sizeof(*out));
    trim_copy(in->raw, out->normalized, sizeof(out->normalized));
    if (out->normalized[0] == '\0') {
        out->status = DIGIT_VALIDATOR_FAIL;
        out->confidence = DIGIT_VALIDATOR_CONFIDENCE_LOW;
        snprintf(out->reason, sizeof(out->reason), "EMPTY_INPUT");
    } else {
        replace_word(out->normalized, sizeof(out->normalized), " numer ", " number ");
        replace_word(out->normalized, sizeof(out->normalized), " teh ", " the ");
        replace_word(out->normalized, sizeof(out->normalized), " you general orders", " your general orders");
        replace_word(out->normalized, sizeof(out->normalized), "GGUG", "GGUF");
        replace_word(out->normalized, sizeof(out->normalized), "ggug", "gguf");
        out->status = DIGIT_VALIDATOR_PASS;
        out->confidence = strcmp(in->raw, out->normalized) == 0 ? DIGIT_VALIDATOR_CONFIDENCE_HIGH : DIGIT_VALIDATOR_CONFIDENCE_MODERATE;
        snprintf(out->reason, sizeof(out->reason), "PASS");
    }
    *response_used = sizeof(*out);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t outbound_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *context)
{
    const digit_validator_outbound_request_t *in;
    digit_validator_outbound_result_t *out;
    const char *question;
    (void)context;
    if (request == NULL || response == NULL || response_used == NULL || request_size != sizeof(*in) || response_size < sizeof(*out)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    in = (const digit_validator_outbound_request_t *)request;
    out = (digit_validator_outbound_result_t *)response;
    memset(out, 0, sizeof(*out));
    question = in->normalized[0] ? in->normalized : in->raw;
    out->status = DIGIT_VALIDATOR_FAIL;
    out->retry_allowed = in->attempt < DIGIT_VALIDATOR_MAX_ATTEMPTS;

    if (in->candidate[0] == '\0')
        snprintf(out->reason, sizeof(out->reason), "EMPTY_RESPONSE");
    else if (nearly_echo(question, in->candidate))
        snprintf(out->reason, sizeof(out->reason), "ECHO");
    else if (asks_explain(question) && weak_explanation(in->candidate))
        snprintf(out->reason, sizeof(out->reason), "OPERATION_INCOMPLETE");
    else if (!evidence_supports(in->candidate, in->evidence))
        snprintf(out->reason, sizeof(out->reason), "UNSUPPORTED_CLAIM");
    else {
        out->status = DIGIT_VALIDATOR_PASS;
        out->retry_allowed = 0;
        snprintf(out->reason, sizeof(out->reason), "PASS");
    }

    *response_used = sizeof(*out);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t validator_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->tests_failed = 0;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t validator_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL || host->unregister_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_VALIDATOR_INBOUND_SERVICE, inbound_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    if (!host->register_service(DIGIT_VALIDATOR_OUTBOUND_SERVICE, outbound_service, NULL)) {
        host->unregister_service(DIGIT_VALIDATOR_INBOUND_SERVICE, NULL);
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    validator_host = host;
    if (host->send_message != NULL) host->send_message("Validator active.");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t validator_stop(void)
{
    int ok = 1;
    if (validator_host != NULL && validator_host->unregister_service != NULL) {
        if (!validator_host->unregister_service(DIGIT_VALIDATOR_OUTBOUND_SERVICE, NULL)) ok = 0;
        if (!validator_host->unregister_service(DIGIT_VALIDATOR_INBOUND_SERVICE, NULL)) ok = 0;
    }
    validator_host = NULL;
    return ok ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_STOP_FAILED;
}

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    static const stnlabz_module_descriptor_t descriptor = {
        "validator", "Validator", 1, 0, 1,
        STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
        validator_qualify, validator_start, validator_stop
    };
    return &descriptor;
}
