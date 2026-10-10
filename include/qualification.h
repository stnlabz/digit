#ifndef DIGIT_QUALIFICATION_H
#define DIGIT_QUALIFICATION_H

#include <stddef.h>

#include "module.h"

/* [AI:GPT-6 | 2026-10-10] Preserve retained module-version qualification history.
 * The prior 128-entry limit exhausted at startup and prevented Interface
 * qualification persistence despite passing qualification. */
#define DIGIT_QUALIFICATION_MAX 4096

typedef struct
{
    char module_id[STNLABZ_MODULE_ID_MAX];
    unsigned int version_major;
    unsigned int version_minor;
    unsigned int version_patch;
} digit_qualification_entry_t;

typedef struct
{
    digit_qualification_entry_t entries[DIGIT_QUALIFICATION_MAX];
    size_t count;
} digit_qualification_inventory_t;

void digit_qualification_init(digit_qualification_inventory_t *inventory);

int digit_qualification_contains(
    const digit_qualification_inventory_t *inventory,
    const stnlabz_module_descriptor_t *descriptor
);

int digit_qualification_record(
    digit_qualification_inventory_t *inventory,
    const stnlabz_module_descriptor_t *descriptor
);

#endif
