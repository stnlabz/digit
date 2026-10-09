#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "audit.h"

static FILE *digit_audit_file = NULL;

/* [AI:GPT-6 | 2026-10-08] Core owns a rolling 8192-event recent
 * history; disk audit history remains append-only and unbounded by
 * this cache. This buffer cannot prevent a module state transition. */
typedef struct {
    unsigned long sequence;
    char module_id[64];
    char event[48];
} digit_lifecycle_entry_t;
static digit_lifecycle_entry_t lifecycle_entries[DIGIT_LIFECYCLE_AUDIT_CAPACITY];
static size_t lifecycle_head, lifecycle_count;
static unsigned long lifecycle_sequence;

size_t digit_audit_lifecycle_count(void) { return lifecycle_count; }
size_t digit_audit_lifecycle_capacity(void) { return DIGIT_LIFECYCLE_AUDIT_CAPACITY; }

void digit_audit_lifecycle(const char *module_id, const char *event, const char *detail)
{
    digit_lifecycle_entry_t *entry;
    char record[384];
    if (!module_id || !event) return;
    entry = &lifecycle_entries[lifecycle_head];
    memset(entry, 0, sizeof(*entry));
    entry->sequence = ++lifecycle_sequence;
    snprintf(entry->module_id, sizeof(entry->module_id), "%s", module_id);
    snprintf(entry->event, sizeof(entry->event), "%s", event);
    lifecycle_head = (lifecycle_head + 1U) % DIGIT_LIFECYCLE_AUDIT_CAPACITY;
    if (lifecycle_count < DIGIT_LIFECYCLE_AUDIT_CAPACITY) ++lifecycle_count;
    snprintf(record, sizeof(record), "sequence=%lu module=%s %s",
             entry->sequence, module_id, detail ? detail : "");
    (void)digit_audit_event("MODULE", event, record);
}


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
