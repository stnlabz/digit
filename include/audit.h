#ifndef DIGIT_AUDIT_H
#define DIGIT_AUDIT_H

#define DIGIT_AUDIT_PATH "/opt/digit/logs/audit.log"

int digit_audit_open(void);
/* Explicit path for isolated test or controlled caller; runtime defaults remain fixed. */
int digit_audit_open_path(const char *path);
void digit_audit_close(void);
int digit_audit_event(const char *component, const char *event, const char *detail);

#endif
