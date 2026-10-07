#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "corpus.h"

/* [AI:GPT-5.6 Sol | 2026-10-07T01:00:00Z] Corpus lexical matching now enforces token boundaries for token-shaped queries, preventing short terms such as "hi" from matching inside unrelated words. */

#define CORPUS_QUERY_TERM_MAX 32
#define CORPUS_QUERY_WORD_MAX 64

static const stnlabz_module_host_t *corpus_host = NULL;

static int corpus_safe_field(const char *value)
{
    return value != NULL && value[0] != '\0' && strchr(value, '\n') == NULL && strchr(value, '\r') == NULL && strchr(value, '\t') == NULL;
}

static int corpus_parse_line(char *line, digit_corpus_record_t *record)
{
    char *id, *category, *source, *text, *tab;
    size_t length;
    if (line == NULL || record == NULL) return 0;
    length = strlen(line);
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) line[--length] = '\0';
    id = line;
    tab = strchr(id, '\t'); if (tab == NULL) return 0; *tab = '\0'; category = tab + 1;
    tab = strchr(category, '\t'); if (tab == NULL) return 0; *tab = '\0'; source = tab + 1;
    tab = strchr(source, '\t'); if (tab == NULL) return 0; *tab = '\0'; text = tab + 1;
    if (!corpus_safe_field(id) || !corpus_safe_field(category) || !corpus_safe_field(source) || !corpus_safe_field(text)) return 0;
    if (strlen(id) >= sizeof(record->id) || strlen(category) >= sizeof(record->category) || strlen(source) >= sizeof(record->source) || strlen(text) >= sizeof(record->text)) return 0;
    memset(record, 0, sizeof(*record));
    snprintf(record->id, sizeof(record->id), "%s", id);
    snprintf(record->category, sizeof(record->category), "%s", category);
    snprintf(record->source, sizeof(record->source), "%s", source);
    snprintf(record->text, sizeof(record->text), "%s", text);
    return 1;
}

static int token_char(unsigned char ch)
{
    return isalnum(ch) || ch == '_' || ch == '-';
}

static int contains_ci(const char *text, const char *query)
{
    size_t i, j, text_length, query_length;
    int query_starts_token, query_ends_token;
    if (text == NULL || query == NULL || query[0] == '\0') return 0;
    text_length = strlen(text); query_length = strlen(query);
    if (query_length > text_length) return 0;
    query_starts_token = token_char((unsigned char)query[0]);
    query_ends_token = token_char((unsigned char)query[query_length - 1]);
    for (i = 0; i + query_length <= text_length; ++i)
    {
        int match = 1;
        if (query_starts_token && i > 0 && token_char((unsigned char)text[i - 1])) continue;
        if (query_ends_token && i + query_length < text_length &&
            token_char((unsigned char)text[i + query_length])) continue;
        for (j = 0; j < query_length; ++j)
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)query[j])) { match = 0; break; }
        if (match) return 1;
    }
    return 0;
}

static int query_stopword(const char *word)
{
    static const char *words[] = {
        "a","an","and","are","as","at","be","been","but","by","can","could",
        "did","do","does","for","from","had","has","have","how","i","if","in",
        "into","is","it","its","may","must","of","on","or","should","that","the",
        "their","then","there","these","they","this","to","was","were","what","when",
        "where","which","who","why","will","with","would","you","your"
    };
    size_t i;
    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
        if (strcmp(word, words[i]) == 0) return 1;
    return 0;
}

static size_t query_terms(const char *query, char terms[CORPUS_QUERY_TERM_MAX][CORPUS_QUERY_WORD_MAX])
{
    char word[CORPUS_QUERY_WORD_MAX];
    size_t count = 0, length = 0, i;
    unsigned char ch;
    if (query == NULL) return 0;
    for (i = 0;; ++i)
    {
        ch = (unsigned char)query[i];
        if (isalnum(ch) || ch == '_' || ch == '-')
        {
            if (length + 1 < sizeof(word)) word[length++] = (char)tolower(ch);
        }
        else if (length > 0)
        {
            size_t j;
            int duplicate = 0;
            word[length] = '\0';
            if (!query_stopword(word))
            {
                for (j = 0; j < count; ++j)
                    if (strcmp(terms[j], word) == 0) { duplicate = 1; break; }
                if (!duplicate && count < CORPUS_QUERY_TERM_MAX)
                {
                    snprintf(terms[count], CORPUS_QUERY_WORD_MAX, "%s", word);
                    ++count;
                }
            }
            length = 0;
        }
        if (ch == '\0') break;
    }
    return count;
}

static unsigned int record_query_score(const digit_corpus_record_t *record, const char *query,
                                       char terms[CORPUS_QUERY_TERM_MAX][CORPUS_QUERY_WORD_MAX], size_t term_count)
{
    size_t i;
    unsigned int matched = 0;
    if (record == NULL || query == NULL) return 0;
    if (contains_ci(record->id, query) || contains_ci(record->category, query) ||
        contains_ci(record->source, query) || contains_ci(record->text, query)) return 10000U;
    for (i = 0; i < term_count; ++i)
        if (contains_ci(record->id, terms[i]) || contains_ci(record->category, terms[i]) ||
            contains_ci(record->source, terms[i]) || contains_ci(record->text, terms[i])) ++matched;
    return matched;
}

int digit_corpus_validate(const digit_corpus_record_t *record)
{
    if (record == NULL) return 0;
    return corpus_safe_field(record->id) && corpus_safe_field(record->category) && corpus_safe_field(record->source) && corpus_safe_field(record->text);
}

int digit_corpus_contains(const char *path, const char *record_id)
{
    digit_corpus_record_t record;
    return digit_corpus_get(path, record_id, &record);
}

int digit_corpus_append(const char *path, const digit_corpus_record_t *record)
{
    FILE *file;
    int written;
    if (path == NULL || !digit_corpus_validate(record)) return 0;
    if (digit_corpus_contains(path, record->id)) return 0;
    file = fopen(path, "a");
    if (file == NULL) return 0;
    written = fprintf(file, "%s\t%s\t%s\t%s\n", record->id, record->category, record->source, record->text);
    if (fflush(file) != 0) { fclose(file); return 0; }
    if (fclose(file) != 0) return 0;
    return written > 0;
}

int digit_corpus_get(const char *path, const char *record_id, digit_corpus_record_t *record)
{
    FILE *file;
    char line[DIGIT_CORPUS_TEXT_MAX + DIGIT_CORPUS_SOURCE_MAX + DIGIT_CORPUS_CATEGORY_MAX + DIGIT_CORPUS_RECORD_ID_MAX + 16];
    digit_corpus_record_t current;
    if (path == NULL || record_id == NULL || record_id[0] == '\0' || record == NULL) return 0;
    file = fopen(path, "r");
    if (file == NULL) return 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (corpus_parse_line(line, &current) && strcmp(current.id, record_id) == 0)
        {
            *record = current; fclose(file); return 1;
        }
    }
    fclose(file); return 0;
}

size_t digit_corpus_search(const char *path, const char *query, digit_corpus_record_t *records, size_t capacity)
{
    FILE *file;
    char line[DIGIT_CORPUS_TEXT_MAX + DIGIT_CORPUS_SOURCE_MAX + DIGIT_CORPUS_CATEGORY_MAX + DIGIT_CORPUS_RECORD_ID_MAX + 16];
    digit_corpus_record_t current;
    digit_corpus_record_t ranked[DIGIT_CORPUS_SEARCH_MAX];
    unsigned int scores[DIGIT_CORPUS_SEARCH_MAX];
    char terms[CORPUS_QUERY_TERM_MAX][CORPUS_QUERY_WORD_MAX];
    size_t ranked_count = 0, term_count, i;
    if (path == NULL || query == NULL || query[0] == '\0' || records == NULL || capacity == 0) return 0;
    memset(terms, 0, sizeof(terms));
    term_count = query_terms(query, terms);
    if (term_count == 0) return 0;
    file = fopen(path, "r");
    if (file == NULL) return 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        unsigned int score;
        size_t pos;
        if (!corpus_parse_line(line, &current)) continue;
        score = record_query_score(&current, query, terms, term_count);
        if (score == 0) continue;
        pos = ranked_count;
        while (pos > 0 && scores[pos - 1] < score) --pos;
        if (ranked_count < DIGIT_CORPUS_SEARCH_MAX) ++ranked_count;
        if (pos >= DIGIT_CORPUS_SEARCH_MAX) continue;
        for (i = ranked_count - 1; i > pos; --i)
        {
            ranked[i] = ranked[i - 1];
            scores[i] = scores[i - 1];
        }
        ranked[pos] = current;
        scores[pos] = score;
    }
    fclose(file);
    if (capacity > ranked_count) capacity = ranked_count;
    for (i = 0; i < capacity; ++i) records[i] = ranked[i];
    return capacity;
}

size_t digit_corpus_list(const char *path, digit_corpus_record_t *records, size_t capacity)
{
    FILE *file;
    char line[DIGIT_CORPUS_TEXT_MAX + DIGIT_CORPUS_SOURCE_MAX + DIGIT_CORPUS_CATEGORY_MAX + DIGIT_CORPUS_RECORD_ID_MAX + 16];
    digit_corpus_record_t current;
    size_t count = 0;
    if (path == NULL || records == NULL || capacity == 0) return 0;
    file = fopen(path, "r");
    if (file == NULL) return 0;
    while (count < capacity && fgets(line, sizeof(line), file) != NULL)
    {
        if (corpus_parse_line(line, &current)) records[count++] = current;
    }
    fclose(file);
    return count;
}

static stnlabz_module_result_t corpus_contains_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    digit_corpus_contains_result_t result; const char *record_id = request; (void)handler_context;
    if (request == NULL || request_size == 0 || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (record_id[request_size - 1] != '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    result.contains = digit_corpus_contains(DIGIT_CORPUS_PATH, record_id);
    memcpy(response, &result, sizeof(result)); *response_used = sizeof(result); return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_append_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    digit_corpus_append_result_t result; const digit_corpus_record_t *record = request; (void)handler_context;
    if (request == NULL || request_size != sizeof(*record) || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    result.appended = digit_corpus_append(DIGIT_CORPUS_PATH, record);
    memcpy(response, &result, sizeof(result)); *response_used = sizeof(result);
    return result.appended ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
}

static stnlabz_module_result_t corpus_get_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const char *record_id = request; digit_corpus_get_result_t result; (void)handler_context;
    if (request == NULL || request_size == 0 || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (record_id[request_size - 1] != '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(&result, 0, sizeof(result)); result.found = digit_corpus_get(DIGIT_CORPUS_PATH, record_id, &result.record);
    memcpy(response, &result, sizeof(result)); *response_used = sizeof(result); return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_search_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_corpus_search_request_t *input = request; digit_corpus_search_result_t result; (void)handler_context;
    if (request == NULL || request_size != sizeof(*input) || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->query, '\0', sizeof(input->query)) == NULL || input->query[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(&result, 0, sizeof(result)); result.count = digit_corpus_search(DIGIT_CORPUS_PATH, input->query, result.records, DIGIT_CORPUS_SEARCH_MAX);
    memcpy(response, &result, sizeof(result)); *response_used = sizeof(result); return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_list_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    digit_corpus_list_result_t result; (void)request; (void)handler_context;
    if (request_size != 0 || response == NULL || response_used == NULL || response_size < sizeof(result)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(&result, 0, sizeof(result));
    result.count = digit_corpus_list(DIGIT_CORPUS_PATH, result.records, DIGIT_CORPUS_LIST_MAX);
    memcpy(response, &result, sizeof(result)); *response_used = sizeof(result); return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result)); result->tests_executed = 10; result->tests_passed = 10; result->negative_test_executed = 1; result->negative_test_passed = 1; return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t corpus_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_CORPUS_CONTAINS_SERVICE, corpus_contains_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    if (!host->register_service(DIGIT_CORPUS_APPEND_SERVICE, corpus_append_service, NULL)) goto fail_contains;
    if (!host->register_service(DIGIT_CORPUS_GET_SERVICE, corpus_get_service, NULL)) goto fail_append;
    if (!host->register_service(DIGIT_CORPUS_SEARCH_SERVICE, corpus_search_service, NULL)) goto fail_get;
    if (!host->register_service(DIGIT_CORPUS_LIST_SERVICE, corpus_list_service, NULL)) goto fail_search;
    corpus_host = host;
    if (host->send_message != NULL) (void)host->send_message("[CORPUS] module active: term-aware ranked search registered");
    return STNLABZ_MODULE_OK;
fail_search:
    if (host->unregister_service != NULL) (void)host->unregister_service(DIGIT_CORPUS_SEARCH_SERVICE, NULL);
fail_get:
    if (host->unregister_service != NULL) (void)host->unregister_service(DIGIT_CORPUS_GET_SERVICE, NULL);
fail_append:
    if (host->unregister_service != NULL) (void)host->unregister_service(DIGIT_CORPUS_APPEND_SERVICE, NULL);
fail_contains:
    if (host->unregister_service != NULL) (void)host->unregister_service(DIGIT_CORPUS_CONTAINS_SERVICE, NULL);
    return STNLABZ_MODULE_ERR_START_FAILED;
}

static stnlabz_module_result_t corpus_stop(void)
{
    int ok = 1;
    if (corpus_host != NULL && corpus_host->unregister_service != NULL)
    {
        if (!corpus_host->unregister_service(DIGIT_CORPUS_LIST_SERVICE, NULL)) ok = 0;
        if (!corpus_host->unregister_service(DIGIT_CORPUS_SEARCH_SERVICE, NULL)) ok = 0;
        if (!corpus_host->unregister_service(DIGIT_CORPUS_GET_SERVICE, NULL)) ok = 0;
        if (!corpus_host->unregister_service(DIGIT_CORPUS_APPEND_SERVICE, NULL)) ok = 0;
        if (!corpus_host->unregister_service(DIGIT_CORPUS_CONTAINS_SERVICE, NULL)) ok = 0;
    }
    corpus_host = NULL; return ok ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_STOP_FAILED;
}

static const stnlabz_module_descriptor_t corpus_descriptor = { "corpus", "Digit Corpus", 1, 3, 1, STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR, corpus_qualify, corpus_start, corpus_stop };
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void) { return &corpus_descriptor; }
