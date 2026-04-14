#include <stdio.h>
#include <sqlite3.h>
#include "../digit.h"

/* EXTERN REFERENCES */
extern weight_node_t weight_matrix[];
extern int weight_count;
extern vocabulary_t vocab[];
extern int vocab_count;

static int train_callback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc > 0 && argv[0]) update_weights(argv[0]);
    return 0;
}

void train_from_source(sqlite3 *db) {
    char *err = 0;
    sqlite3_exec(db, "SELECT content FROM ingested_data;", train_callback, 0, &err);
    if (err) { printf("error: %s\n", err); sqlite3_free(err); }
    else { save_weights_to_bin(); }
}

void save_weights_to_bin() {
    FILE *f = fopen("data/weights.bin", "wb");
    if (!f) return;

    // Save metadata first: how many weights and how many vocab words
    fwrite(&weight_count, sizeof(int), 1, f);
    fwrite(&vocab_count, sizeof(int), 1, f);

    // Save the actual data structures
    fwrite(weight_matrix, sizeof(weight_node_t), weight_count, f);
    fwrite(vocab, sizeof(vocabulary_t), vocab_count, f);

    fclose(f);
    printf("digit: %d weights and %d words saved to data/weights.bin\n", weight_count, vocab_count);
}
