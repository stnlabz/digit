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
void stitch_response(const char *query, char chunks[][MAX_LEN], int count, char *out)
{
    if (count <= 0) {
        strcpy(out, "No relevant context found.");
        return;
    }

    // clean all chunks
    for (int i = 0; i < count; i++) {
        clean_text(chunks[i]);
    }

    // score chunks
    int best = 0;
    int best_score = score_chunk(chunks[0], query);

    for (int i = 1; i < count; i++) {
        int s = score_chunk(chunks[i], query);
        if (s > best_score) {
            best = i;
            best_score = s;
        }
    }

    // try merge with next best if similar
    for (int i = 0; i < count; i++) {
        if (i == best) continue;

        if (similar_chunks(chunks[best], chunks[i])) {
            merge_chunks(chunks[best], chunks[i], out);
            enforce_output_limit(out);
            return;
        }
    }

    // fallback: best chunk only
    strncpy(out, chunks[best], MAX_LEN - 1);
    out[MAX_LEN - 1] = '\0';

    enforce_output_limit(out);
}
