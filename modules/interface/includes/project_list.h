#ifndef DIGIT_PROJECT_LIST_H
#define DIGIT_PROJECT_LIST_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-09] Organization-scoped project listing.
 * Caller must separately verify the authenticated organization SA.
 * Invalid or incomplete project inventories fail closed. */
int digit_project_list_scoped(const char *root,const char *organization,const char *identity,
                             char *output,size_t capacity);
#endif
