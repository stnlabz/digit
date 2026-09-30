#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIELD_MAX 8192

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s OUTPUT.jsonl CONTEXT QUESTION ANSWER\n", program);
}

static int valid_field(const char *value)
{
    return value != NULL && value[0] != '\0' && strlen(value) < FIELD_MAX;
}

static int json_write_string(FILE *stream, const char *value)
{
    const unsigned char *p = (const unsigned char *)value;
    if (fputc('"', stream) == EOF) return 0;
    while (*p != '\0')
    {
        switch (*p)
        {
            case '"': if (fputs("\\\"", stream) == EOF) return 0; break;
            case '\\': if (fputs("\\\\", stream) == EOF) return 0; break;
            case '\b': if (fputs("\\b", stream) == EOF) return 0; break;
            case '\f': if (fputs("\\f", stream) == EOF) return 0; break;
            case '\n': if (fputs("\\n", stream) == EOF) return 0; break;
            case '\r': if (fputs("\\r", stream) == EOF) return 0; break;
            case '\t': if (fputs("\\t", stream) == EOF) return 0; break;
            default:
                if (*p < 0x20)
                {
                    if (fprintf(stream, "\\u%04x", (unsigned int)*p) < 0) return 0;
                }
                else if (fputc((int)*p, stream) == EOF) return 0;
                break;
        }
        ++p;
    }
    return fputc('"', stream) != EOF;
}

int main(int argc, char **argv)
{
    FILE *stream;
    if (argc != 5)
    {
        usage(argv[0]);
        return 2;
    }
    if (!valid_field(argv[2]) || !valid_field(argv[3]) || !valid_field(argv[4]))
    {
        fprintf(stderr, "context, question, and answer must be non-empty and smaller than %d bytes\n", FIELD_MAX);
        return 2;
    }

    stream = fopen(argv[1], "ab");
    if (stream == NULL)
    {
        perror("fopen");
        return 1;
    }

    if (fputs("{\"context\":", stream) == EOF || !json_write_string(stream, argv[2]) ||
        fputs(",\"question\":", stream) == EOF || !json_write_string(stream, argv[3]) ||
        fputs(",\"answer\":", stream) == EOF || !json_write_string(stream, argv[4]) ||
        fputs("}\n", stream) == EOF)
    {
        fprintf(stderr, "failed to write training example\n");
        fclose(stream);
        return 1;
    }

    if (fclose(stream) != 0)
    {
        perror("fclose");
        return 1;
    }

    printf("TRAINING_EXAMPLE_APPENDED output=%s\n", argv[1]);
    return 0;
}
