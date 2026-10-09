#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "audit.h"

static FILE *digit_audit_file = NULL;

/* [AI:GPT-6 | 2026-10-08] Isolated unit tests open a private sink;
 * production callers continue to use the fixed audit pathname. */
int digit_audit_open_path(const char *path)
{
    if (digit_audit_file != NULL)
    {
        return 1;
    }

    if (path == NULL || path[0] != '/')
    {
        return 0;
    }

    digit_audit_file = fopen(path, "a");
    if (digit_audit_file == NULL)
    {
        return 0;
    }

    (void)setvbuf(digit_audit_file, NULL, _IOLBF, 0);
    return 1;
}

int digit_audit_open(void)
{
    return digit_audit_open_path(DIGIT_AUDIT_PATH);
}

void digit_audit_close(void)
{
    if (digit_audit_file == NULL)
    {
        return;
    }

    (void)fflush(digit_audit_file);
    (void)fclose(digit_audit_file);
    digit_audit_file = NULL;
}

int digit_audit_event(const char *component, const char *event, const char *detail)
{
    time_t now;
    struct tm timestamp;
    char time_buffer[32];

    if (digit_audit_file == NULL || component == NULL || event == NULL)
    {
        return 0;
    }

    now = time(NULL);
    if (now == (time_t)-1 || gmtime_r(&now, &timestamp) == NULL)
    {
        return 0;
    }

    if (strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%SZ", &timestamp) == 0)
    {
        return 0;
    }

    if (detail == NULL || detail[0] == '\0')
    {
        return fprintf(digit_audit_file, "%s %s %s\n", time_buffer, component, event) > 0;
    }

    return fprintf(digit_audit_file, "%s %s %s %s\n", time_buffer, component, event, detail) > 0;
}

/* [AI:GPT-6 | 2026-10-08] Synchronize checkpoint records to disk
 * before Core can reclaim in-memory ABI lifecycle audit slots. */
int digit_audit_sync(void)
{
    if (digit_audit_file == NULL) return 0;
    if (fflush(digit_audit_file) != 0) return 0;
    return fsync(fileno(digit_audit_file)) == 0;
}
