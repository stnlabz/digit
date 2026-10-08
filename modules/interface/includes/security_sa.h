#ifndef DIGIT_SECURITY_SA_H
#define DIGIT_SECURITY_SA_H

/* [AI:GPT-6 | 2026-10-08]
 * Operator-controlled SA assignments; separate from general administration.
 * Format per record: user<TAB>organization<TAB>SA<TAB>qualification(0|1)
 * <TAB>assignment(0|1)<TAB>mission_qualification(0|1)<NEWLINE>
 * Only a fully verified SA in the same organization qualifies for #security.
 */
#define DIGIT_SECURITY_SA_REGISTRY "/opt/digit/state/auth/security_sa.tsv"
int digit_security_sa_verify(const char *registry, const char *organization,
                             const char *identity);
#endif
