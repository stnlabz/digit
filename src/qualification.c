#include <stdio.h>
#include <string.h>

#include "qualification.h"

void digit_qualification_init(digit_qualification_inventory_t *inventory)
{
    if (inventory != NULL)
    {
        memset(inventory, 0, sizeof(*inventory));
    }
}

int digit_qualification_contains(
    const digit_qualification_inventory_t *inventory,
    const stnlabz_module_descriptor_t *descriptor
)
{
    size_t index;

    if (inventory == NULL || descriptor == NULL)
    {
        return 0;
    }

    for (index = 0; index < inventory->count; ++index)
    {
        const digit_qualification_entry_t *entry = &inventory->entries[index];

        if (strcmp(entry->module_id, descriptor->id) == 0 &&
            entry->version_major == descriptor->version_major &&
            entry->version_minor == descriptor->version_minor &&
            entry->version_patch == descriptor->version_patch)
        {
            return 1;
        }
    }

    return 0;
}

int digit_qualification_record(
    digit_qualification_inventory_t *inventory,
    const stnlabz_module_descriptor_t *descriptor
)
{
    digit_qualification_entry_t *entry;

    if (inventory == NULL || descriptor == NULL || descriptor->id[0] == '\0')
    {
        return 0;
    }

    if (digit_qualification_contains(inventory, descriptor))
    {
        return 1;
    }

    if (inventory->count >= DIGIT_QUALIFICATION_MAX)
    {
        return 0;
    }

    entry = &inventory->entries[inventory->count++];
    memset(entry, 0, sizeof(*entry));
    snprintf(entry->module_id, sizeof(entry->module_id), "%s", descriptor->id);
    entry->version_major = descriptor->version_major;
    entry->version_minor = descriptor->version_minor;
    entry->version_patch = descriptor->version_patch;
    return 1;
}
