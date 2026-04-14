#ifndef DIGIT_H
#define DIGIT_H

#include <stdio.h>
#include <stddef.h>
#include <sqlite3.h>

/* --- Shared Communication Buffer --- */
/* This is where all core output is stored for the Web API to pick up */
extern char digit_out_buffer[4096];

extern int logged_in;
extern long current_session_id;

/* --- Tokenizer & Integrity --- */
typedef enum {
    TOKEN_LITERAL,
    TOKEN_VIOLATION,
    TOKEN_ENTITY,
    TOKEN_CMD,
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type;
    char text[256];
    int id; 
} token_t;

/* --- Command Architecture --- */
typedef struct {
    const char *name;
    const char *description;
    int (*handler)(const char *);
    int restricted;
    int min_role;
} command_t;

/* --- Brain & LLM Weights --- */
#define MAX_TOKENS 5000
#define MAX_TOKEN_LEN 64

typedef struct {
    char text[MAX_TOKEN_LEN];
    int id;
} vocabulary_t;

typedef struct {
    int from_token_id;
    int to_token_id;
    int weight;
} weight_node_t;

/* --- Global Declarations --- */
extern command_t restricted_commands[];
extern sqlite3 *db; // Shared DB handle

/* --- Function Prototypes --- */

// Tokenizer & Security (core/tokenizer.c)
token_t get_next_token(const char **input);
int validate_integrity(const char *data);

// Command Management (core/restricted.c)
int get_restricted_count();
int cmd_rules(const char *args);
int cmd_ingest(const char *args);
int cmd_chat(const char *args);
int cmd_think(const char *args);
int cmd_exit(const char *args);
void add_command(char *name, void (*handler)(char *), char *desc, int role);

// Neural/LLM Logic (core/llm_trainer.c & core/inference.c)
void train_from_source(sqlite3 *db);
void update_weights(const char *content);
void save_weights_to_bin();
void digit_speak(const char *input);
int find_heaviest_connection(int from_id);
const char* get_word_from_id(int id);
void load_weights_from_bin();

/* Utility Handlers */
int handle_study(const char *args);
int handle_save(const char *args);

#endif
