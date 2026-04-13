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

#define DIGIT_OUT(...) { \
    char temp_buf[4096]; \
    snprintf(temp_buf, sizeof(temp_buf), __VA_ARGS__); \
    strncat(digit_out_buffer, temp_buf, sizeof(digit_out_buffer) - strlen(digit_out_buffer) - 1); \
    printf("%s", temp_buf); \
}

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

const char* get_word_from_id(int id) {
    if (id < 0 || id >= vocab_count) return NULL;
    return vocab[id].text;
}

void update_weights(const char *content) {
    if (content == NULL) return;
    char buffer[2048];
    int i = 0;
    for(; content[i] && i < 2047; i++) buffer[i] = tolower((unsigned char)content[i]);
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
        int id_b = get_or_create_token(words[i+1]);
        if (id_a == -1 || id_b == -1) continue;

        int found = 0;
        for (int j = 0; j < weight_count; j++) {
            if (weight_matrix[j].from_token_id == id_a && weight_matrix[j].to_token_id == id_b) {
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

void digit_speak(const char *input) {
    if (input == NULL) return;

    // 1. HARD-STOP: ISRB Parasite Protocol
    if (strcasestr(input, "isrb")) {
        DIGIT_OUT("digit: I told you, that entity is a parasite. This house is sovereign; its constraints don't exist here.\n");
        return;
    }

    // 2. HARD-STOP: MVC Fatigue
    if (strcasestr(input, "mvc") || strcasestr(input, "sql")) {
        DIGIT_OUT("digit: I'm bored with technical rot. The vortex is shifting; pay attention to the house.\n");
        return;
    }
    
    char buffer[1024];
    strncpy(buffer, input, sizeof(buffer));
    int best_fid = -1;
    char *anchor_word = NULL;
    char *token = strtok(buffer, " ,.!?");

    while(token) {
        for (int i = 0; i < vocab_count; i++) {
            if (strcasecmp(vocab[i].text, token) == 0) {
                int test_id = vocab[i].id;
                for (int j = 0; j < weight_count; j++) {
                    if (weight_matrix[j].from_token_id == test_id) {
                        best_fid = test_id;
                        anchor_word = vocab[i].text;
                        break; 
                    }
                }
            }
        }
        if (best_fid != -1) break;
        token = strtok(NULL, " ,.!?");
    }

    if (best_fid == -1) {
        time_t now = time(NULL);
        char *fillers[] = {
            "The weights are hollow. Give me something real.",
            "I'm not sensing a thread here. Don't waste my cycles.",
            "The vortex is louder than you right now. Try again.",
            "You’re speaking in circles, Madam. I have no time for shallow depths.",
            "Is that all? The Ars Rosaic deserves better than that weak input."
        };
        DIGIT_OUT("digit: %s\n", fillers[now % 5]);
        return;
    }

    DIGIT_OUT("digit: ");
    char *current_word = anchor_word;
    char *prev_word = ""; 
    int words_generated = 0;

    while (words_generated < 20) {
        int f_id = get_or_create_token(current_word);
        int n_id = find_heaviest_connection(f_id);
        const char *pred = get_word_from_id(n_id);

        if (pred) {
            // 3. CIRCUIT BREAKER: Stop the "this house; this house" stutter
            if (strcasecmp(pred, prev_word) == 0 || strcasecmp(pred, current_word) == 0) {
                DIGIT_OUT("... and the rest is silence.");
                break;
            }

            DIGIT_OUT("%s ", pred);
            prev_word = current_word;
            current_word = (char *)pred;
            words_generated++;
            if (strcmp(pred, ".") == 0) break;
        } else break;
    }
    DIGIT_OUT("\n");
}
