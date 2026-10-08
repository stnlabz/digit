#ifndef DIGIT_PROJECT_PROVISION_H
#define DIGIT_PROJECT_PROVISION_H

#include <stddef.h>

/* [AI:GPT-6 | 2026-10-08] A project is visible only after its complete
 * restricted security workspace and verified SA membership are committed.
 * This facility does NOT create legacy flat Core channels or open HTTP access.
 */
#define DIGIT_PROJECT_ROOT "/opt/digit/state/projects"
#define DIGIT_PROJECT_ID_MAX 64
#define DIGIT_PROJECT_USER_MAX 64

int digit_project_provision(const char *root, const char *organization,
                            const char *project, const char *founding_admin,
                            const char *security_sa,
                            const char *sa_registry);
int digit_project_security_ready(const char *root, const char *organization,
                                  const char *project);

#endif
