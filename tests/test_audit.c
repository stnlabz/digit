#include <stdio.h>

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
    check(DIGIT_AUDIT_PATH[0] == '/', "audit path is absolute");
    check(digit_audit_event("TEST", "BEFORE_OPEN", NULL) == 0, "write before open is rejected");
    check(digit_audit_open(), "audit log opens");
    check(digit_audit_open(), "duplicate open is harmless");
    check(digit_audit_event("TEST", "EVENT", "detail=value"), "audit event writes");
    check(digit_audit_event("TEST", "EMPTY", ""), "empty detail writes");
    check(digit_audit_event("TEST", "NULL_DETAIL", NULL), "null detail writes");
    check(digit_audit_event(NULL, "BAD", NULL) == 0, "null component is rejected");
    check(digit_audit_event("TEST", NULL, NULL) == 0, "null event is rejected");
    digit_audit_close();
    check(digit_audit_event("TEST", "AFTER_CLOSE", NULL) == 0, "write after close is rejected");

    printf("\nAudit tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
