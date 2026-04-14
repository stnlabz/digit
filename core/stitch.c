/**
 * stitch.c
 * Controlled response stitching for Digit
 * No inference. No hallucination. Strict assembly only.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_CHUNKS 5
#define MAX_LEN 1024

// --- CLEAN TEXT ---
void clean_text(char *text)
{
    if (!text) return;

    // remove newlines
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n' || text[i] == '\r')
            text[i] = ' ';
    }

    // collapse spaces
    char tmp[MAX_LEN];
    int j = 0, space = 0;

    for (int i = 0; text[i] && j < MAX_LEN - 1; i++) {
        if (isspace(text[i])) {
            if (!space) tmp[j++] = ' ';
            space = 1;
        } else {
            tmp[j++] = text[i];
            space = 0;
        }
    }

    tmp[j] = '\0';
    strcpy(text, tmp);
}

// --- SCORE CHUNK (simple keyword overlap) ---
int score_chunk(const char *chunk, const char *query)
{
    int score = 0;

    char q_copy[MAX_LEN];
    strncpy(q_copy, query, MAX_LEN - 1);
    q_copy[MAX_LEN - 1] = '\0';

    char *token = strtok(q_copy, " ");
    while (token) {
        if (strstr(chunk, token))
            score++;
        token = strtok(NULL, " ");
    }

    return score;
}

// --- SIMILARITY CHECK ---
int similar_chunks(const char *a, const char *b)
{
    int overlap = 0;

    char a_copy[MAX_LEN];
    strncpy(a_copy, a, MAX_LEN - 1);
    a_copy[MAX_LEN - 1] = '\0';

    char *token = strtok(a_copy, " ");
    while (token) {
        if (strstr(b, token))
            overlap++;
        token = strtok(NULL, " ");
    }

    return overlap > 3; // threshold
}

// --- MERGE TWO CHUNKS ---
void merge_chunks(const char *a, const char *b, char *out)
{
    snprintf(out, MAX_LEN, "%s %s", a, b);
}

// --- TRIM TO 1-2 SENTENCES ---
void enforce_output_limit(char *text)
{
    int sentences = 0;

    for (int i = 0; text[i]; i++) {
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            sentences++;
            if (sentences >= 2) {
                text[i + 1] = '\0';
                return;
            }
        }
    }
}

// --- MAIN STITCH FUNCTION ---
void stitch_response(const char *input, char *output)
{
    if (!input || !output) return;

    output[0] = '\0';

    // --- STEP 1: safe copy ---
    char temp[1024] = {0};
    strncpy(temp, input, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    // --- STEP 2: collapse multiple spaces ---
    char cleaned[1024] = {0};
    int j = 0;
    for (int i = 0; temp[i] != '\0' && j < 1023; i++) {
        if (!(temp[i] == ' ' && temp[i+1] == ' ')) {
            cleaned[j++] = temp[i];
        }
    }
    cleaned[j] = '\0';

    // --- STEP 3: trim leading space ---
    char *start = cleaned;
    while (*start == ' ') start++;

    // --- STEP 4: remove weak leading tokens ---
    const char *weak[] = {"you", "in", "that", "the"};
    char *words[64];
    int count = 0;

    char buffer[1024];
    strncpy(buffer, start, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *tok = strtok(buffer, " ");
    while (tok && count < 64) {
        words[count++] = tok;
        tok = strtok(NULL, " ");
    }

    int shift = 0;
    if (count > 1) {
        for (int i = 0; i < 4; i++) {
            if (strcasecmp(words[0], weak[i]) == 0) {
                shift = 1;
                break;
            }
        }
    }

    // --- STEP 5: rebuild sentence ---
    char rebuilt[1024] = {0};
    for (int i = shift; i < count; i++) {
        strncat(rebuilt, words[i], sizeof(rebuilt) - strlen(rebuilt) - 2);
        if (i < count - 1) {
            strncat(rebuilt, " ", sizeof(rebuilt) - strlen(rebuilt) - 2);
        }
    }

    // --- STEP 6: capitalize first letter ---
    if (rebuilt[0] != '\0') {
        rebuilt[0] = toupper(rebuilt[0]);
    }

    // --- STEP 7: ensure ending punctuation ---
    size_t len = strlen(rebuilt);
    if (len > 0) {
        char last = rebuilt[len - 1];
        if (last != '.' && last != '!' && last != '?') {
            strncat(rebuilt, ".", sizeof(rebuilt) - len - 1);
        }
    }

    // --- FINAL COPY ---
    strncpy(output, rebuilt, 1023);
    output[1023] = '\0';
}
