#include <stdio.h>
#include <string.h>

#include "qualification_store.h"

int digit_qualification_store_load(
    const char *path,
    digit_qualification_inventory_t *inventory
)
{
    FILE *file;
    char line[512];

    if (path == NULL || inventory == NULL)
    {
        return 0;
    }

    digit_qualification_init(inventory);
    file = fopen(path, "r");
    if (file == NULL)
    {
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        stnlabz_module_descriptor_t descriptor;
        char id[STNLABZ_MODULE_ID_MAX];
        unsigned int major;
        unsigned int minor;
        unsigned int patch;
        char extra;

        if (line[0] == '\n' || line[0] == '#')
        {
            continue;
        }

        if (sscanf(line, "%63[^|]|%u|%u|%u%c",
                   id, &major, &minor, &patch, &extra) != 4)
        {
            fclose(file);
            digit_qualification_init(inventory);
            return 0;
        }

        memset(&descriptor, 0, sizeof(descriptor));
        snprintf(descriptor.id, sizeof(descriptor.id), "%s", id);
        descriptor.version_major = major;
        descriptor.version_minor = minor;
        descriptor.version_patch = patch;

        if (!digit_qualification_record(inventory, &descriptor))
        {
            fclose(file);
            digit_qualification_init(inventory);
            return 0;
        }
    }

    if (ferror(file))
    {
        fclose(file);
        digit_qualification_init(inventory);
        return 0;
    }

    fclose(file);
    return 1;
}

int digit_qualification_store_save(
    const char *path,
    const digit_qualification_inventory_t *inventory
)
{
    FILE *file;
    size_t index;

    if (path == NULL || inventory == NULL)
    {
        return 0;
    }

    file = fopen(path, "w");
    if (file == NULL)
    {
        return 0;
    }

    for (index = 0; index < inventory->count; ++index)
    {
        const digit_qualification_entry_t *entry = &inventory->entries[index];

        if (fprintf(file, "%s|%u|%u|%u\n",
                    entry->module_id,
                    entry->version_major,
                    entry->version_minor,
                    entry->version_patch) < 0)
        {
            fclose(file);
            return 0;
        }
    }

    if (fclose(file) != 0)
    {
        return 0;
    }

    return 1;
}
