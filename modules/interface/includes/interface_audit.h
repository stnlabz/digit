#ifndef DIGIT_INTERFACE_AUDIT_H
#define DIGIT_INTERFACE_AUDIT_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-08] Interface 1.3.9.
 * Structured and payload-free request audit events.
 * Only route category and HTTP status are recorded. */
int digit_interface_audit_format(const char *request,int status,
                                 char *output,size_t capacity);
#endif
