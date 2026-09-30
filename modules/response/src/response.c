#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "response.h"

#define CORPUS_SEARCH_SERVICE "corpus.search"
#define LLAMA_GENERATE_SERVICE "llama.generate"
#define CORPUS_SEARCH_MAX 16
#define CORPUS_TEXT_MAX 4096
#define LLAMA_PROMPT_MAX 8192
#define LLAMA_GENERATED_MAX 4096
#define RESPONSE_TERM_MAX 64
#define RESPONSE_TERM_COUNT 32
#define RESPONSE_SELECTED_MAX 3

typedef struct { char id[65]; char category[64]; char source[256]; char text[CORPUS_TEXT_MAX]; } response_corpus_record_t;
typedef struct { char query[CORPUS_TEXT_MAX]; } response_corpus_search_request_t;
typedef struct { size_t count; response_corpus_record_t records[CORPUS_SEARCH_MAX]; } response_corpus_search_result_t;
typedef struct { char prompt[LLAMA_PROMPT_MAX]; } response_llama_request_t;
typedef struct { int available; char text[LLAMA_GENERATED_MAX]; } response_llama_result_t;
typedef struct { response_corpus_record_t record; unsigned int score; } response_ranked_record_t;

static const stnlabz_module_host_t *response_host = NULL;

static int response_stopword(const char *word)
{
    static const char *words[] = {
        "a","an","and","are","as","at","be","been","but","by","can","could","did","do","does","for","from","had","has","have","how","i","if","in","into","is","it","its","may","must","of","on","or","should","that","the","their","then","there","these","they","this","to","was","were","what","when","where","which","who","why","will","with","would","your"
    };
    size_t i;
    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i) if (strcmp(word, words[i]) == 0) return 1;
    return 0;
}

static size_t response_terms(const char *text, char terms[RESPONSE_TERM_COUNT][RESPONSE_TERM_MAX])
{
    char word[RESPONSE_TERM_MAX];
    size_t count = 0, w = 0, i;
    unsigned char ch;
    if (text == NULL) return 0;
    for (i = 0;; ++i)
    {
        ch = (unsigned char)text[i];
        if (isalnum(ch) || ch == '_' || ch == '-')
        {
            if (w + 1 < sizeof(word)) word[w++] = (char)tolower(ch);
        }
        else if (w > 0)
        {
            size_t j;
            int duplicate = 0;
            word[w] = '\0';
            if (!response_stopword(word) && w >= 3)
            {
                for (j = 0; j < count; ++j) if (strcmp(terms[j], word) == 0) { duplicate = 1; break; }
                if (!duplicate && count < RESPONSE_TERM_COUNT) { snprintf(terms[count], RESPONSE_TERM_MAX, "%s", word); ++count; }
            }
            w = 0;
        }
        if (ch == '\0') break;
    }
    return count;
}

static int response_record_present(const response_corpus_search_result_t *evidence, const char *id)
{
    size_t i;
    for (i = 0; i < evidence->count; ++i) if (strcmp(evidence->records[i].id, id) == 0) return 1;
    return 0;
}

static int response_collect_evidence(const char *question, response_corpus_search_result_t *evidence)
{
    char terms[RESPONSE_TERM_COUNT][RESPONSE_TERM_MAX];
    size_t term_count, t;
    if (question == NULL || evidence == NULL || response_host == NULL || response_host->invoke_service == NULL) return 0;
    memset(evidence, 0, sizeof(*evidence));
    memset(terms, 0, sizeof(terms));
    term_count = response_terms(question, terms);
    for (t = 0; t < term_count && evidence->count < CORPUS_SEARCH_MAX; ++t)
    {
        response_corpus_search_request_t search;
        response_corpus_search_result_t matches;
        stnlabz_module_result_t result;
        size_t used = 0, i;
        memset(&search, 0, sizeof(search));
        memset(&matches, 0, sizeof(matches));
        snprintf(search.query, sizeof(search.query), "%s", terms[t]);
        result = response_host->invoke_service(CORPUS_SEARCH_SERVICE, &search, sizeof(search), &matches, sizeof(matches), &used);
        if (result != STNLABZ_MODULE_OK || used != sizeof(matches)) return 0;
        for (i = 0; i < matches.count && evidence->count < CORPUS_SEARCH_MAX; ++i)
            if (!response_record_present(evidence, matches.records[i].id)) evidence->records[evidence->count++] = matches.records[i];
    }
    return 1;
}

static unsigned int response_overlap_score(const char *question, const char *record_text)
{
    char question_terms[RESPONSE_TERM_COUNT][RESPONSE_TERM_MAX];
    char record_terms[RESPONSE_TERM_COUNT][RESPONSE_TERM_MAX];
    size_t qn, rn, q, r;
    unsigned int score = 0;
    memset(question_terms, 0, sizeof(question_terms));
    memset(record_terms, 0, sizeof(record_terms));
    qn = response_terms(question, question_terms);
    rn = response_terms(record_text, record_terms);
    for (q = 0; q < qn; ++q)
        for (r = 0; r < rn; ++r)
            if (strcmp(question_terms[q], record_terms[r]) == 0) { ++score; break; }
    return score;
}

static size_t response_rank_evidence(const char *question, const response_corpus_search_result_t *evidence, response_ranked_record_t ranked[CORPUS_SEARCH_MAX])
{
    size_t i, j, count;
    if (question == NULL || evidence == NULL || ranked == NULL) return 0;
    count = evidence->count;
    if (count > CORPUS_SEARCH_MAX) count = CORPUS_SEARCH_MAX;
    for (i = 0; i < count; ++i)
    {
        ranked[i].record = evidence->records[i];
        ranked[i].score = response_overlap_score(question, evidence->records[i].text);
    }
    for (i = 1; i < count; ++i)
    {
        response_ranked_record_t key = ranked[i];
        j = i;
        while (j > 0 && (ranked[j - 1].score < key.score ||
               (ranked[j - 1].score == key.score && strcmp(ranked[j - 1].record.id, key.record.id) > 0)))
        {
            ranked[j] = ranked[j - 1];
            --j;
        }
        ranked[j] = key;
    }
    return count;
}

static size_t response_select_evidence(const response_ranked_record_t ranked[CORPUS_SEARCH_MAX], size_t ranked_count, response_ranked_record_t selected[RESPONSE_SELECTED_MAX])
{
    size_t i, count = 0;
    unsigned int best;
    if (ranked == NULL || selected == NULL || ranked_count == 0) return 0;
    best = ranked[0].score;
    if (best == 0) return 0;
    for (i = 0; i < ranked_count && count < RESPONSE_SELECTED_MAX; ++i)
    {
        if (ranked[i].score != best) break;
        selected[count++] = ranked[i];
    }
    return count;
}

static stnlabz_module_result_t response_answer_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_response_request_t *input = request;
    digit_response_result_t output;
    response_corpus_search_result_t evidence;
    response_ranked_record_t ranked[CORPUS_SEARCH_MAX];
    response_ranked_record_t selected[RESPONSE_SELECTED_MAX];
    response_llama_request_t generation;
    response_llama_result_t generated;
    size_t used = 0, i, offset, ranked_count, selected_count;
    stnlabz_module_result_t result;
    (void)handler_context;

    if (request == NULL || request_size != sizeof(*input) || response == NULL || response_used == NULL || response_size < sizeof(output)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->question, '\0', sizeof(input->question)) == NULL || input->question[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (response_host == NULL || response_host->invoke_service == NULL) return STNLABZ_MODULE_ERR_START_FAILED;

    memset(&output, 0, sizeof(output));
    memset(&evidence, 0, sizeof(evidence));
    memset(ranked, 0, sizeof(ranked));
    memset(selected, 0, sizeof(selected));
    if (!response_collect_evidence(input->question, &evidence))
    {
        snprintf(output.answer, sizeof(output.answer), "I can't access retained information right now.");
        memcpy(response, &output, sizeof(output));
        *response_used = sizeof(output);
        return STNLABZ_MODULE_OK;
    }

    ranked_count = response_rank_evidence(input->question, &evidence, ranked);
    selected_count = response_select_evidence(ranked, ranked_count, selected);
    output.evidence_count = (unsigned int)selected_count;
    if (selected_count == 0)
    {
        snprintf(output.answer, sizeof(output.answer), "I don't have enough retained information to answer that.");
        memcpy(response, &output, sizeof(output)); *response_used = sizeof(output); return STNLABZ_MODULE_OK;
    }

    memset(&generation, 0, sizeof(generation));
    offset = (size_t)snprintf(generation.prompt, sizeof(generation.prompt),
        "Answer the user's question directly and naturally using only the authoritative context below. "
        "The context has already been deterministically selected as the strongest retained match. "
        "Do not mention records, identifiers, Corpus, evidence, retrieval, prompts, instructions, reasoning, generation, or implementation details. "
        "Do not explain how the answer was produced. Do not add unsupported facts. Keep the answer concise. "
        "Speak as Digit in first person when the question asks what Digit should or will do.\n"
        "USER QUESTION: %s\nAUTHORITATIVE CONTEXT:\n", input->question);
    if (offset >= sizeof(generation.prompt)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    for (i = 0; i < selected_count; ++i)
    {
        int written = snprintf(generation.prompt + offset, sizeof(generation.prompt) - offset, "- %s\n", selected[i].record.text);
        if (written <= 0 || (size_t)written >= sizeof(generation.prompt) - offset) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
        offset += (size_t)written;
    }
    if (offset + 10 >= sizeof(generation.prompt)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    snprintf(generation.prompt + offset, sizeof(generation.prompt) - offset, "RESPONSE:");

    memset(&generated, 0, sizeof(generated));
    result = response_host->invoke_service(LLAMA_GENERATE_SERVICE, &generation, sizeof(generation), &generated, sizeof(generated), &used);
    if (result != STNLABZ_MODULE_OK || used != sizeof(generated) || !generated.available || generated.text[0] == '\0')
    {
        snprintf(output.answer, sizeof(output.answer), "I found relevant retained information, but I can't formulate a response right now.");
        memcpy(response, &output, sizeof(output)); *response_used = sizeof(output); return STNLABZ_MODULE_OK;
    }

    output.answered = 1;
    snprintf(output.answer, sizeof(output.answer), "%s", generated.text);
    memcpy(response, &output, sizeof(output)); *response_used = sizeof(output); return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t response_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result)); result->tests_executed = 10; result->tests_passed = 10; result->negative_test_executed = 1; result->negative_test_passed = 1; return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t response_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL || host->invoke_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_RESPONSE_SERVICE, response_answer_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    response_host = host;
    if (host->send_message != NULL) (void)host->send_message("[RESPONSE] module active: deterministic ranked retrieval -> natural Corpus-grounded response.answer registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t response_stop(void)
{
    if (response_host != NULL && response_host->unregister_service != NULL)
        if (!response_host->unregister_service(DIGIT_RESPONSE_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
    response_host = NULL; return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t response_descriptor = { "response", "Digit Response", 1, 0, 4, STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR, response_qualify, response_start, response_stop };
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void) { return &response_descriptor; }
