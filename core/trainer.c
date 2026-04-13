#include <stdio.h>
#include <stdlib.h>
#include "../digit.h"

void digit_train_from_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Could not open %s for training.\n", filename);
        return;
    }

    char line[1024];
    int lines_processed = 0;

    printf("Digit is indexing %s...\n", filename);

    while (fgets(line, sizeof(line), fp)) {
        // Feed each line into the existing weight updater
        update_weights(line);
        lines_processed++;
    }

    fclose(fp);
    printf("Training complete. %d lines indexed into memory.\n", lines_processed);
}
