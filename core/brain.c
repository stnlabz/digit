#include "../digit.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>

/* GLOBAL BRAIN STATE */
weight_node_t weight_matrix[MAX_TOKENS];
int weight_count = 0;
vocabulary_t vocab[MAX_TOKENS];
int vocab_count = 0;

/* ========================= */
/* INTENT DETECTION */
/* ========================= */
int is_recall_query(const char *input) {
    if (!input) return 0;

    return (
        strcasestr(input, "what did i say") ||
        strcasestr(input, "did i say") ||
        strcasestr(input, "what were you told") ||
        strcasestr(input, "remember")
    );
}

/* ========================= */
/* KEYWORD EXTRACTION */
/* ========================= */
void extract_keyword(const char *input, char *keyword, size_t size) {
    if (!input || !keyword) return;

    char buffer[256];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = strtok(buffer, " ,.!?");
    while (token) {
        if (
            strlen(token) > 3 &&
            strcasecmp(token, "what") &&
            strcasecmp(token, "did") &&
            strcasecmp(token, "say") &&
            strcasecmp(token, "about")
        ) {
            strncpy(keyword, token, size - 1);
            return;
        }
        token = strtok(NULL, " ,.!?");
    }
}

/* ========================= */
/* DB LOOKUP */
/* ========================= */
int query_relevant_knowledge(const char *input, char *output, size_t size) {
    if (!db || !input || !output) return 0;

    char keyword[128] = {0};
    extract_keyword(input, keyword, sizeof(keyword));

    if (strlen(keyword) == 0) return 0;

    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT rule_text AS text FROM rules WHERE rule_text LIKE ? "
        "UNION ALL "
        "SELECT content AS text FROM ingested_data WHERE content LIKE ? "
        "LIMIT 5;";

    char like_query[256];
    snprintf(like_query, sizeof(like_query), "%%%s%%", keyword);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, like_query, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, like_query, -1, SQLITE_STATIC);

    int found = 0;
    output[0] = '\0';

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text) {
            strncat(output, (const char *)text, size - strlen(output) - 2);
            strncat(output, " ", size - strlen(output) - 2);
            found = 1;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

/* ========================= */
/* WEIGHT SYSTEM */
/* ========================= */

int get_or_create_token(const char *text) {
    for (int i = 0; i < vocab_count; i++) {
        if (strcasecmp(vocab[i].text, text) == 0) return vocab[i].id;
    }

    if (vocab_count < MAX_TOKENS) {
        strncpy(vocab[vocab_count].text, text, 63);
        vocab[vocab_count].id = vocab_count;
        return vocab_count++;
    }

    return -1;
}

int get_token_id(const char *text) {
    for (int i = 0; i < vocab_count; i++) {
        if (strcasecmp(vocab[i].text, text) == 0) return vocab[i].id;
    }
    return -1;
}

const char* get_word_from_id(int id) {
    if (id < 0 || id >= vocab_count) return NULL;
    return vocab[id].text;
}

int find_heaviest_connection(int from_id) {
    int candidates[10];
    int count = 0;

    for (int i = 0; i < weight_count && count < 10; i++) {
        if (weight_matrix[i].from_token_id == from_id) {
            candidates[count++] = weight_matrix[i].to_token_id;
        }
    }

    if (count == 0) return -1;
    return candidates[rand() % count];
}

/* ========================= */
/* LOAD WEIGHTS */
/* ========================= */
void load_weights_from_bin() {
    FILE *fp = fopen("data/weights.bin", "rb");
    if (fp == NULL) {
        fp = fopen("weights.bin", "rb");
    }

    if (fp) {
        size_t read = fread(neural_weights, sizeof(float), MAX_WEIGHTS, fp);
        WEIGHT_COUNT = (int)read;
        fclose(fp);
        printf("[SYSTEM] Loaded %d weights from bin. Intelligence active.\n", WEIGHT_COUNT);
    } else {
        printf("[WARNING] weights.bin not found. Running on zero-state logic.\n");
    }
}

/* ========================= */
/* UPDATE WEIGHTS */
/* ========================= */
void update_weights(const char *content) {
    if (content == NULL) return;

    char buffer[2048];
    int i = 0;

    for (; content[i] && i < 2047; i++) {
        buffer[i] = tolower((unsigned char)content[i]);
    }
    buffer[i] = '\0';

    char *words[256];
    int w_count = 0;

    char *token = strtok(buffer, " ./:{}[]\",\r\n");

    while (token != NULL && w_count < 256) {
        words[w_count++] = token;
        token = strtok(NULL, " ./:{}[]\",\r\n");
    }

    if (w_count < 2) return;

    for (int i = 0; i < w_count - 1; i++) {
        int id_a = get_or_create_token(words[i]);
        int id_b = get_or_create_token(words[i + 1]);

        if (id_a == -1 || id_b == -1) continue;

        int found = 0;

        for (int j = 0; j < weight_count; j++) {
            if (weight_matrix[j].from_token_id == id_a &&
                weight_matrix[j].to_token_id == id_b) {
                weight_matrix[j].weight++;
                found = 1;
                break;
            }
        }

        if (!found && weight_count < MAX_TOKENS) {
            weight_matrix[weight_count].from_token_id = id_a;
            weight_matrix[weight_count].to_token_id = id_b;
            weight_matrix[weight_count].weight = 1;
            weight_count++;
        }
    }
}

/* ========================= */
/* OUTPUT */
/* ========================= */
#define DIGIT_OUT(...) { \
    char temp_buf[4096]; \
    snprintf(temp_buf, sizeof(temp_buf), __VA_ARGS__); \
    strncat(digit_out_buffer, temp_buf, sizeof(digit_out_buffer) - strlen(digit_out_buffer) - 1); \
    printf("%s", temp_buf); \
}

/* ========================= */
/* MAIN */
/* ========================= */
void digit_speak(const char *input) {
    if (input == NULL) return;

    /* RECALL → DB FIRST */
    if (is_recall_query(input)) {

        char db_result[2048] = {0};

        if (query_relevant_knowledge(input, db_result, sizeof(db_result))) {
            char final[2048] = {0};
            stitch_response(db_result, final);
            DIGIT_OUT("digit: %s\n", final);
            return;
        }

        /* FALLBACK → WEIGHTS */
        char keyword[128] = {0};
        extract_keyword(input, keyword, sizeof(keyword));

        int id = get_token_id(keyword);

        if (id != -1) {
            int next = find_heaviest_connection(id);

            if (next != -1) {
                const char *start = get_word_from_id(next);

               if (start) {
    char raw[1024] = {0};
    char final[1024] = {0};

    char *current_word = (char *)start;
    char *prev_word = "";
    int words_generated = 0;

    while (words_generated < 20) {
        int f_id = get_token_id(current_word);
        int n_id = find_heaviest_connection(f_id);
        const char *pred = get_word_from_id(n_id);

        if (!pred) break;

        if (strcasecmp(pred, prev_word) == 0 ||
            strcasecmp(pred, current_word) == 0) {
            break;
        }

        strncat(raw, pred, sizeof(raw) - strlen(raw) - 2);
        strncat(raw, " ", sizeof(raw) - strlen(raw) - 2);

        prev_word = current_word;
        current_word = (char *)pred;
        words_generated++;
    }

    stitch_response(raw, final);
    DIGIT_OUT("digit: %s\n", final);
    return;
}
            }
        }

        DIGIT_OUT("digit: I have no record of that.\n");
        return;
    }

    /* NORMAL MODE */
    char buffer[1024];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = strtok(buffer, " ,.!?");
    int best_fid = -1;
    char *anchor_word = NULL;

    while (token) {
        int id = get_token_id(token);

        if (id != -1) {
            for (int j = 0; j < weight_count; j++) {
                if (weight_matrix[j].from_token_id == id) {
                    best_fid = id;
                    anchor_word = vocab[id].text;
                    break;
                }
            }
        }

        if (best_fid != -1) break;
        token = strtok(NULL, " ,.!?");
    }

    if (best_fid == -1) {
        DIGIT_OUT("digit: The weights are hollow. Give me something real.\n");
        return;
    }

    char raw[1024] = {0};
    char final[1024] = {0};

    char *current_word = anchor_word;
    char *prev_word = "";
    int words_generated = 0;

    while (words_generated < 20) {
        int f_id = get_token_id(current_word);
        int n_id = find_heaviest_connection(f_id);
        const char *pred = get_word_from_id(n_id);

        if (!pred) break;

        if (strcasecmp(pred, prev_word) == 0 ||
            strcasecmp(pred, current_word) == 0) {
            strncat(raw, "...", sizeof(raw) - strlen(raw) - 1);
            break;
        }

        strncat(raw, pred, sizeof(raw) - strlen(raw) - 2);
        strncat(raw, " ", sizeof(raw) - strlen(raw) - 2);

        prev_word = current_word;
        current_word = (char *)pred;
        words_generated++;
    }

    stitch_response(raw, final);
    DIGIT_OUT("digit: %s\n", final);
}
