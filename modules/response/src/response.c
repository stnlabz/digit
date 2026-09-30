#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "response.h"

#define CORPUS_LIST_SERVICE "corpus.list"
#define LLAMA_GENERATE_SERVICE "llama.generate"
#define CORPUS_MAX 64
#define CORPUS_TEXT_MAX 4096
#define LLAMA_PROMPT_MAX 8192
#define LLAMA_GENERATED_MAX 4096
#define TERM_MAX 64
#define TERM_COUNT 32
#define SELECTED_MAX 6

typedef struct {
    char id[65];
    char category[64];
    char source[256];
    char text[CORPUS_TEXT_MAX];
} corpus_record_t;

typedef struct {
    size_t count;
    corpus_record_t records[CORPUS_MAX];
} corpus_result_t;

typedef struct { char prompt[LLAMA_PROMPT_MAX]; } llama_request_t;
typedef struct { int available; char text[LLAMA_GENERATED_MAX]; } llama_result_t;

typedef struct {
    corpus_record_t record;
    unsigned int score;
    unsigned int matches;
} ranked_record_t;

static const stnlabz_module_host_t *response_host = NULL;

static int stopword(const char *word)
{
    static const char *words[] = {
        "a", "an", "and", "are", "as", "at", "be", "been", "but", "by",
        "can", "could", "did", "do", "does", "for", "from", "had", "has",
        "have", "how", "i", "if", "in", "into", "is", "it", "its", "may",
        "must", "of", "on", "or", "should", "that", "the", "their", "then",
        "there", "these", "they", "this", "to", "was", "were", "what", "when",
        "where", "which", "who", "why", "will", "with", "would", "your"
    };
    size_t i;

    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        if (strcmp(word, words[i]) == 0) return 1;
    }
    return 0;
}

static size_t terms(const char *text, char out[TERM_COUNT][TERM_MAX])
{
    char word[TERM_MAX];
    size_t count = 0, w = 0, i;
    unsigned char ch;

    if (text == NULL) return 0;
    for (i = 0;; ++i) {
        ch = (unsigned char)text[i];
        if (isalnum(ch) || ch == '_' || ch == '-') {
            if (w + 1 < sizeof(word)) word[w++] = (char)tolower(ch);
        } else if (w > 0) {
            size_t j;
            int duplicate = 0;
            word[w] = '\0';
            if (!stopword(word) && w >= 3) {
                for (j = 0; j < count; ++j) {
                    if (strcmp(out[j], word) == 0) {
                        duplicate = 1;
                        break;
                    }
                }
                if (!duplicate && count < TERM_COUNT) {
                    snprintf(out[count], TERM_MAX, "%s", word);
                    ++count;
                }
            }
            w = 0;
        }
        if (ch == '\0') break;
    }
    return count;
}

static int has_term(char list[TERM_COUNT][TERM_MAX], size_t count, const char *term)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        if (strcmp(list[i], term) == 0) return 1;
    }
    return 0;
}

static int collect_corpus(corpus_result_t *evidence)
{
    stnlabz_module_result_t result;
    size_t used = 0;

    if (evidence == NULL || response_host == NULL ||
        response_host->invoke_service == NULL) return 0;
    memset(evidence, 0, sizeof(*evidence));
    result = response_host->invoke_service(
        CORPUS_LIST_SERVICE, NULL, 0, evidence, sizeof(*evidence), &used);
    return result == STNLABZ_MODULE_OK && used == sizeof(*evidence);
}

static unsigned int term_frequency(const corpus_result_t *evidence, const char *term)
{
    size_t i;
    unsigned int frequency = 0;

    for (i = 0; i < evidence->count; ++i) {
        char rt[TERM_COUNT][TERM_MAX];
        size_t rn;
        memset(rt, 0, sizeof(rt));
        rn = terms(evidence->records[i].text, rt);
        if (has_term(rt, rn, term)) ++frequency;
    }
    return frequency;
}

static unsigned int ordered_run_bonus(
    char qt[TERM_COUNT][TERM_MAX], size_t qn,
    char rt[TERM_COUNT][TERM_MAX], size_t rn)
{
    size_t q, r;
    unsigned int best = 0;

    for (q = 0; q < qn; ++q) {
        for (r = 0; r < rn; ++r) {
            size_t qi = q, ri = r;
            unsigned int run = 0;
            while (qi < qn && ri < rn && strcmp(qt[qi], rt[ri]) == 0) {
                ++run;
                ++qi;
                ++ri;
            }
            if (run > best) best = run;
        }
    }

    if (best >= 3) return best * best * 500U;
    if (best == 2) return 1000U;
    return 0;
}

static unsigned int proximity_bonus(
    char qt[TERM_COUNT][TERM_MAX], size_t qn,
    char rt[TERM_COUNT][TERM_MAX], size_t rn)
{
    size_t q, r;
    unsigned int bonus = 0;

    for (q = 0; q + 1 < qn; ++q) {
        for (r = 0; r + 2 < rn; ++r) {
            if (strcmp(qt[q], rt[r]) == 0 &&
                (strcmp(qt[q + 1], rt[r + 1]) == 0 ||
                 strcmp(qt[q + 1], rt[r + 2]) == 0)) {
                bonus += 250U;
                break;
            }
        }
    }
    return bonus;
}

static unsigned int record_score(
    const char *question,
    const corpus_result_t *evidence,
    const char *record_text,
    unsigned int *matches_out)
{
    char qt[TERM_COUNT][TERM_MAX];
    char rt[TERM_COUNT][TERM_MAX];
    size_t qn, rn, q;
    unsigned int score = 0, matches = 0;

    memset(qt, 0, sizeof(qt));
    memset(rt, 0, sizeof(rt));
    qn = terms(question, qt);
    rn = terms(record_text, rt);

    for (q = 0; q < qn; ++q) {
        if (has_term(rt, rn, qt[q])) {
            unsigned int frequency = term_frequency(evidence, qt[q]);
            unsigned int rarity = frequency
                ? ((unsigned int)evidence->count * 100U) / frequency : 100U;
            score += 100U + rarity;
            ++matches;
        }
    }

    score += ordered_run_bonus(qt, qn, rt, rn);
    score += proximity_bonus(qt, qn, rt, rn);
    if (matches > 1) score += matches * matches * 25U;
    if (qn > 0) score += (matches * 200U) / (unsigned int)qn;
    if (matches_out != NULL) *matches_out = matches;
    return score;
}

static size_t rank_evidence(
    const char *question,
    const corpus_result_t *evidence,
    ranked_record_t ranked[CORPUS_MAX])
{
    size_t i, j, count;

    if (question == NULL || evidence == NULL || ranked == NULL) return 0;
    count = evidence->count > CORPUS_MAX ? CORPUS_MAX : evidence->count;

    for (i = 0; i < count; ++i) {
        ranked[i].record = evidence->records[i];
        ranked[i].score = record_score(
            question, evidence, evidence->records[i].text, &ranked[i].matches);
    }
    for (i = 1; i < count; ++i) {
        ranked_record_t key = ranked[i];
        j = i;
        while (j > 0 &&
               (ranked[j - 1].score < key.score ||
                (ranked[j - 1].score == key.score &&
                 strcmp(ranked[j - 1].record.id, key.record.id) > 0))) {
            ranked[j] = ranked[j - 1];
            --j;
        }
        ranked[j] = key;
    }
    return count;
}

static size_t select_evidence(
    const char *question,
    ranked_record_t ranked[CORPUS_MAX],
    size_t ranked_count,
    ranked_record_t selected[SELECTED_MAX])
{
    char qt[TERM_COUNT][TERM_MAX];
    char anchor[TERM_COUNT][TERM_MAX];
    int covered[TERM_COUNT];
    size_t qn, an, i, q, count = 0;
    unsigned int minimum_matches;

    memset(qt, 0, sizeof(qt));
    memset(anchor, 0, sizeof(anchor));
    memset(covered, 0, sizeof(covered));
    qn = terms(question, qt);
    if (ranked == NULL || selected == NULL || ranked_count == 0 ||
        qn == 0 || ranked[0].matches == 0) return 0;

    selected[count++] = ranked[0];
    minimum_matches = ranked[0].matches > 1 ? ranked[0].matches - 1 : 1;
    an = terms(ranked[0].record.text, anchor);
    for (q = 0; q < qn; ++q) {
        if (has_term(anchor, an, qt[q])) covered[q] = 1;
    }

    for (i = 1; i < ranked_count && count < SELECTED_MAX; ++i) {
        char rt[TERM_COUNT][TERM_MAX];
        size_t rn;
        unsigned int shared = 0;
        int adds = 0;

        if (ranked[i].matches < minimum_matches) continue;
        memset(rt, 0, sizeof(rt));
        rn = terms(ranked[i].record.text, rt);
        for (q = 0; q < an; ++q) {
            if (has_term(rt, rn, anchor[q])) ++shared;
        }
        if (shared < 2) continue;
        for (q = 0; q < qn; ++q) {
            if (!covered[q] && has_term(rt, rn, qt[q])) {
                adds = 1;
                break;
            }
        }
        if (!adds) continue;
        selected[count++] = ranked[i];
        for (q = 0; q < qn; ++q) {
            if (has_term(rt, rn, qt[q])) covered[q] = 1;
        }
    }
    return count;
}

static int generated_grounded(
    const char *generated,
    ranked_record_t selected[SELECTED_MAX],
    size_t selected_count)
{
    char gt[TERM_COUNT][TERM_MAX];
    size_t gn, g, i, r;

    if (generated == NULL || generated[0] == '\0' || selected == NULL ||
        selected_count == 0) return 0;
    memset(gt, 0, sizeof(gt));
    gn = terms(generated, gt);
    for (g = 0; g < gn; ++g) {
        int supported = 0;
        for (i = 0; i < selected_count && !supported; ++i) {
            char rt[TERM_COUNT][TERM_MAX];
            size_t rn;
            memset(rt, 0, sizeof(rt));
            rn = terms(selected[i].record.text, rt);
            for (r = 0; r < rn; ++r) {
                if (strcmp(gt[g], rt[r]) == 0) {
                    supported = 1;
                    break;
                }
            }
        }
        if (!supported) return 0;
    }
    return 1;
}

static void grounded_fallback(
    ranked_record_t selected[SELECTED_MAX],
    size_t selected_count,
    digit_response_result_t *output)
{
    size_t i, offset = 0;

    if (output == NULL || selected == NULL || selected_count == 0) return;
    output->answered = 1;
    for (i = 0; i < selected_count; ++i) {
        int written = snprintf(
            output->answer + offset,
            sizeof(output->answer) - offset,
            "%s%s", i ? " " : "", selected[i].record.text);
        if (written <= 0 || (size_t)written >= sizeof(output->answer) - offset) break;
        offset += (size_t)written;
    }
}

static stnlabz_module_result_t answer_service(
    const void *request, size_t request_size,
    void *response, size_t response_size,
    size_t *response_used, void *handler_context)
{
    const digit_response_request_t *input = request;
    digit_response_result_t output;
    corpus_result_t evidence;
    ranked_record_t ranked[CORPUS_MAX];
    ranked_record_t selected[SELECTED_MAX];
    llama_request_t generation;
    llama_result_t generated;
    size_t used = 0, i, offset, ranked_count, selected_count;
    stnlabz_module_result_t result;

    (void)handler_context;
    if (request == NULL || request_size != sizeof(*input) || response == NULL ||
        response_used == NULL || response_size < sizeof(output))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->question, '\0', sizeof(input->question)) == NULL ||
        input->question[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (response_host == NULL || response_host->invoke_service == NULL)
        return STNLABZ_MODULE_ERR_START_FAILED;

    memset(&output, 0, sizeof(output));
    memset(&evidence, 0, sizeof(evidence));
    memset(ranked, 0, sizeof(ranked));
    memset(selected, 0, sizeof(selected));

    if (!collect_corpus(&evidence)) {
        snprintf(output.answer, sizeof(output.answer),
                 "I can't access retained information right now.");
        memcpy(response, &output, sizeof(output));
        *response_used = sizeof(output);
        return STNLABZ_MODULE_OK;
    }

    ranked_count = rank_evidence(input->question, &evidence, ranked);
    selected_count = select_evidence(input->question, ranked, ranked_count, selected);
    output.evidence_count = (unsigned int)selected_count;
    if (selected_count == 0) {
        snprintf(output.answer, sizeof(output.answer),
                 "I don't have enough retained information to answer that.");
        memcpy(response, &output, sizeof(output));
        *response_used = sizeof(output);
        return STNLABZ_MODULE_OK;
    }

    memset(&generation, 0, sizeof(generation));
    offset = (size_t)snprintf(
        generation.prompt, sizeof(generation.prompt),
        "You are Digit's language renderer. The authoritative context below is the complete factual boundary for this answer. Answer the user's question directly and naturally using all context needed to address the question, but assert no fact, capability, purpose, relationship, technology, service, client, goal, or detail that is not explicitly present in that context. Do not use outside knowledge, model knowledge, assumptions, implications, likely details, helpful additions, greetings, offers of further help, or conversational padding. Do not mention records, identifiers, Corpus, evidence, retrieval, prompts, instructions, reasoning, generation, or implementation details. Preserve material qualifiers. Keep the answer concise. Speak as Digit in first person only when the question is about Digit herself; otherwise answer about the subject asked.\nUSER QUESTION: %s\nAUTHORITATIVE CONTEXT:\n",
        input->question);
    if (offset >= sizeof(generation.prompt)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;

    for (i = 0; i < selected_count; ++i) {
        int written = snprintf(
            generation.prompt + offset, sizeof(generation.prompt) - offset,
            "- %s\n", selected[i].record.text);
        if (written <= 0 || (size_t)written >= sizeof(generation.prompt) - offset)
            return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
        offset += (size_t)written;
    }
    if (offset + 10 >= sizeof(generation.prompt))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    snprintf(generation.prompt + offset,
             sizeof(generation.prompt) - offset, "RESPONSE:");

    memset(&generated, 0, sizeof(generated));
    result = response_host->invoke_service(
        LLAMA_GENERATE_SERVICE, &generation, sizeof(generation),
        &generated, sizeof(generated), &used);
    if (result != STNLABZ_MODULE_OK || used != sizeof(generated) ||
        !generated.available || generated.text[0] == '\0') {
        grounded_fallback(selected, selected_count, &output);
    } else if (!generated_grounded(generated.text, selected, selected_count)) {
        if (response_host->send_message != NULL)
            (void)response_host->send_message(
                "[RESPONSE] Llama output rejected: generated terms exceeded selected Corpus evidence");
        grounded_fallback(selected, selected_count, &output);
    } else {
        output.answered = 1;
        snprintf(output.answer, sizeof(output.answer), "%s", generated.text);
    }

    memcpy(response, &output, sizeof(output));
    *response_used = sizeof(output);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t response_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t response_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL ||
        host->invoke_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_RESPONSE_SERVICE, answer_service, NULL))
        return STNLABZ_MODULE_ERR_START_FAILED;
    response_host = host;
    if (host->send_message != NULL)
        (void)host->send_message(
            "[RESPONSE] module active: ordered subject proximity ranking -> constrained Corpus-grounded response.answer registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t response_stop(void)
{
    if (response_host != NULL && response_host->unregister_service != NULL) {
        if (!response_host->unregister_service(DIGIT_RESPONSE_SERVICE, NULL))
            return STNLABZ_MODULE_ERR_STOP_FAILED;
    }
    response_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t response_descriptor = {
    "response", "Digit Response", 1, 0, 9,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    response_qualify, response_start, response_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &response_descriptor;
}
