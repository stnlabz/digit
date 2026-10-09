#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"

/* [AI:GPT-6 | 2026-10-08] 1.3.4 only: protected pathname consistency.
 * Historical regression coverage remains in test_project_provision. */
static unsigned int executed, failed;
static void check(int ok, const char *name)
{
    ++executed;
    if (ok) printf("PASS 1.3.4 %02u - %s\n", executed, name);
    else { ++failed; printf("FAIL 1.3.4 %02u - %s\n", executed, name); }
}

int main(void)
{
    char root[] = "/tmp/digit-interface-134-XXXXXX";
    char project[256], metadata[320], ready[320], alias[350], registry[320];
    FILE *file;
    if (mkdtemp(root) == NULL) return 1;
    snprintf(project, sizeof(project), "%s/stn-labz/digit", root);
    snprintf(metadata, sizeof(metadata), "%s/project.tsv", project);
    snprintf(ready, sizeof(ready), "%s/READY", project);
    snprintf(registry, sizeof(registry), "%s/sa.tsv", root);
    file = fopen(registry, "w");
    if (file == NULL) return 1;
    fputs("sysadmin\tstn-labz\tSA\t1\t1\t1\n", file);
    if (fclose(file) != 0 || chmod(registry, 0600) != 0) return 1;

    check(digit_project_provision(root, "stn-labz", "digit", "poe",
                                  "sysadmin", registry), "fixture provision");
    check(digit_project_security_ready(root, "stn-labz", "digit"),
          "baseline READY and metadata accepted");

    snprintf(alias, sizeof(alias), "%s/metadata-alias", project);
    check(rename(metadata, alias) == 0, "metadata moved");
    check(symlink(alias, metadata) == 0, "metadata alias created");
    check(!digit_project_security_ready(root, "stn-labz", "digit"),
          "metadata alias denied");
    check(unlink(metadata) == 0 && rename(alias, metadata) == 0,
          "metadata restored");
    check(digit_project_security_ready(root, "stn-labz", "digit"),
          "restored metadata accepted");

    snprintf(alias, sizeof(alias), "%s/ready-alias", project);
    check(rename(ready, alias) == 0, "READY moved");
    check(symlink(alias, ready) == 0, "READY alias created");
    check(!digit_project_security_ready(root, "stn-labz", "digit"),
          "READY alias denied");
    check(unlink(ready) == 0 && rename(alias, ready) == 0,
          "READY restored");
    check(digit_project_security_ready(root, "stn-labz", "digit"),
          "restored READY accepted");

    printf("Interface 1.3.4 milestone: %u executed, %u failed\n",
           executed, failed);
    return failed ? 1 : 0;
}
