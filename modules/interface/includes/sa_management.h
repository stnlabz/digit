#ifndef DIGIT_SA_MANAGEMENT_H
#define DIGIT_SA_MANAGEMENT_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-09] Explicit same-organization SA governance.
 * Assignment never manufactures qualification or cross-organization access. */
int digit_sa_list(const char *registry,const char *org,const char *actor,char *json,size_t size);
int digit_sa_change(const char *registry,const char *org,const char *actor,
                    const char *user,int assign);
#endif
