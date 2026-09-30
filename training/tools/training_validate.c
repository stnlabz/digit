#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX_BYTES 32768

static const char *skip_ws(const char *p)
{
    while (*p != '\0' && isspace((unsigned char)*p)) ++p;
    return p;
}

static int consume_string(const char **cursor, int require_nonempty)
{
    const char *p = *cursor;
    size_t decoded = 0;
    if (*p != '"') return 0;
    ++p;
    while (*p != '\0' && *p != '"')
    {
        unsigned char ch = (unsigned char)*p++;
        if (ch < 0x20) return 0;
        if (ch == '\\')
        {
            int i;
            if (*p == '"' || *p == '\\' || *p == '/' || *p == 'b' || *p == 'f' || *p == 'n' || *p == 'r' || *p == 't') ++p;
            else if (*p == 'u')
            {
                ++p;
                for (i = 0; i < 4; ++i) if (!isxdigit((unsigned char)*p++)) return 0;
            }
            else return 0;
        }
        ++decoded;
    }
    if (*p != '"' || (require_nonempty && decoded == 0)) return 0;
    *cursor = p + 1;
    return 1;
}

static int consume_literal(const char **cursor, const char *literal)
{
    size_t n = strlen(literal);
    if (strncmp(*cursor, literal, n) != 0) return 0;
    *cursor += n;
    return 1;
}

static int valid_line(const char *line)
{
    const char *p = skip_ws(line);
    if (*p++ != '{') return 0;
    p = skip_ws(p);
    if (!consume_literal(&p, "\"context\"")) return 0;
    p = skip_ws(p); if (*p++ != ':') return 0; p = skip_ws(p);
    if (!consume_string(&p, 1)) return 0;
    p = skip_ws(p); if (*p++ != ',') return 0; p = skip_ws(p);
    if (!consume_literal(&p, "\"question\"")) return 0;
    p = skip_ws(p); if (*p++ != ':') return 0; p = skip_ws(p);
    if (!consume_string(&p, 1)) return 0;
    p = skip_ws(p); if (*p++ != ',') return 0; p = skip_ws(p);
    if (!consume_literal(&p, "\"answer\"")) return 0;
    p = skip_ws(p); if (*p++ != ':') return 0; p = skip_ws(p);
    if (!consume_string(&p, 1)) return 0;
    p = skip_ws(p); if (*p++ != '}') return 0;
    p = skip_ws(p);
    return *p == '\0';
}

int main(int argc, char **argv)
{
    FILE *stream;
    char line[LINE_MAX_BYTES];
    unsigned long line_number = 0, valid = 0, invalid = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s DATASET.jsonl\n", argv[0]);
        return 2;
    }
    stream = fopen(argv[1], "rb");
    if (stream == NULL) { perror("fopen"); return 1; }

    while (fgets(line, sizeof(line), stream) != NULL)
    {
        size_t n;
        ++line_number;
        n = strlen(line);
        if (n > 0 && line[n - 1] == '\n') line[--n] = '\0';
        if (n > 0 && line[n - 1] == '\r') line[--n] = '\0';
        if (n == sizeof(line) - 1 && !feof(stream))
        {
            int ch;
            while ((ch = fgetc(stream)) != '\n' && ch != EOF) { }
            fprintf(stderr, "INVALID line=%lu reason=line_too_large\n", line_number);
            ++invalid;
            continue;
        }
        if (valid_line(line)) ++valid;
        else { fprintf(stderr, "INVALID line=%lu reason=malformed_or_empty_required_field\n", line_number); ++invalid; }
    }
    if (ferror(stream)) { perror("fgets"); fclose(stream); return 1; }
    fclose(stream);

    printf("TRAINING_VALIDATION lines=%lu valid=%lu invalid=%lu\n", line_number, valid, invalid);
    return invalid == 0 && valid > 0 ? 0 : 1;
}
