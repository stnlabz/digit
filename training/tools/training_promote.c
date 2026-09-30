#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX_BYTES 32768

static int copy_stream(FILE *input, FILE *output, unsigned long *lines)
{
    char buffer[LINE_MAX_BYTES];
    while (fgets(buffer, sizeof(buffer), input) != NULL)
    {
        size_t n = strlen(buffer);
        if (n == 0) continue;
        if (n == sizeof(buffer) - 1 && buffer[n - 1] != '\n' && !feof(input)) return 0;
        if (fputs(buffer, output) == EOF) return 0;
        ++*lines;
    }
    return !ferror(input) && !ferror(output);
}

int main(int argc, char **argv)
{
    FILE *candidate;
    FILE *dataset;
    unsigned long lines = 0;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s CANDIDATES.jsonl DATASET.jsonl\n", argv[0]);
        return 2;
    }

    candidate = fopen(argv[1], "rb");
    if (candidate == NULL) { perror("candidate fopen"); return 1; }

    dataset = fopen(argv[2], "ab");
    if (dataset == NULL) { perror("dataset fopen"); fclose(candidate); return 1; }

    if (!copy_stream(candidate, dataset, &lines))
    {
        fprintf(stderr, "failed to promote candidate data\n");
        fclose(candidate);
        fclose(dataset);
        return 1;
    }

    if (fclose(candidate) != 0 || fclose(dataset) != 0)
    {
        perror("fclose");
        return 1;
    }

    printf("TRAINING_PROMOTION promoted=%lu source=%s destination=%s\n", lines, argv[1], argv[2]);
    return lines > 0 ? 0 : 1;
}
