#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "audit.h"

static unsigned int executed = 0;
static unsigned int failed = 0;

static void check(int condition, const char *name)
{
    ++executed;
    if (condition)
    {
        printf("PASS %02u - %s\n", executed, name);
    }
    else
    {
        ++failed;
        printf("FAIL %02u - %s\n", executed, name);
    }
}

int main(void)
{
    char directory[] = "/tmp/digit-audit-XXXXXX";
    char path[sizeof(directory) + 16];
    if (mkdtemp(directory) == NULL) return 1;
    if (snprintf(path, sizeof(path), "%s/audit.log", directory) >= (int)sizeof(path))
        return 1;
    check(DIGIT_AUDIT_PATH[0] == '/', "audit path is absolute");
    check(digit_audit_event("TEST", "BEFORE_OPEN", NULL) == 0, "write before open is rejected");
    check(digit_audit_open_path(path), "audit log opens");
    check(digit_audit_open_path(path), "duplicate open is harmless");
    check(digit_audit_event("TEST", "EVENT", "detail=value"), "audit event writes");
    check(digit_audit_event("TEST", "EMPTY", ""), "empty detail writes");
    check(digit_audit_event("TEST", "NULL_DETAIL", NULL), "null detail writes");
    check(digit_audit_event(NULL, "BAD", NULL) == 0, "null component is rejected");
    check(digit_audit_event("TEST", NULL, NULL) == 0, "null event is rejected");
    /* [AI:GPT-6 | 2026-10-08] Core lifecycle audit remains writable
     * well beyond the retired 128-event ABI ceiling. */
    check(digit_audit_lifecycle_capacity() == 8192U,
          "Core lifecycle buffer holds 8192 entries");
    for (unsigned int i = 0; i < 8200U; ++i)
        digit_audit_lifecycle("sacrificial", "ACTIVATED", "source=core");
    check(digit_audit_lifecycle_count() == 8192U,
          "Core ring retains 8192 most recent events");
    check(digit_audit_sync(), "persistent lifecycle history flushes");
    {
        FILE *in = fopen(path, "rb");
        unsigned long lines = 0;
        int ch;
        if (in) {
            while ((ch = fgetc(in)) != EOF) if (ch == '\\n') ++lines;
            fclose(in);
        }
        check(lines >= 8200UL, "persistent audit retains events beyond ring capacity");
    }
    digit_audit_close();
    check(digit_audit_event("TEST", "AFTER_CLOSE", NULL) == 0, "write after close is rejected");

    (void)unlink(path);
    (void)rmdir(directory);
    printf("\nAudit tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
