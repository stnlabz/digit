#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "hotload.h"

static int digit_hotload_collect(
    digit_hotload_t *hotload,
    digit_hotload_file_t *files,
    size_t *count
)
{
    DIR *root;
    struct dirent *entry;

    if (hotload == NULL || hotload->manager == NULL || files == NULL || count == NULL)
    {
        return 0;
    }

    *count = 0;
    root = opendir(digit_module_manager_path(hotload->manager));
    if (root == NULL)
    {
        return 0;
    }

    while ((entry = readdir(root)) != NULL)
    {
        char path[DIGIT_HOTLOAD_PATH_MAX];
        char so_path[DIGIT_HOTLOAD_PATH_MAX];
        struct stat status;
        int written;

        if (entry->d_name[0] == '.')
        {
            continue;
        }

        written = snprintf(path, sizeof(path), "%s/%s",
                           digit_module_manager_path(hotload->manager), entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(path) ||
            stat(path, &status) != 0 || !S_ISDIR(status.st_mode))
        {
            continue;
        }

        written = snprintf(so_path, sizeof(so_path), "%s/%s.so", path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(so_path) ||
            stat(so_path, &status) != 0 || !S_ISREG(status.st_mode))
        {
            continue;
        }

        if (*count >= DIGIT_HOTLOAD_MAX_MODULES)
        {
            closedir(root);
            return 0;
        }

        snprintf(files[*count].module_id, sizeof(files[*count].module_id), "%s", entry->d_name);
        snprintf(files[*count].path, sizeof(files[*count].path), "%s", so_path);
        files[*count].modified_time = status.st_mtime;
        files[*count].size = status.st_size;
        ++(*count);
    }

    closedir(root);
    return 1;
}

static const digit_hotload_file_t *digit_hotload_find(
    const digit_hotload_file_t *files,
    size_t count,
    const char *module_id
)
{
    size_t index;

    for (index = 0; index < count; ++index)
    {
        if (strcmp(files[index].module_id, module_id) == 0)
        {
            return &files[index];
        }
    }

    return NULL;
}

void digit_hotload_init(
    digit_hotload_t *hotload,
    digit_module_manager_t *manager
)
{
    if (hotload == NULL)
    {
        return;
    }

    memset(hotload, 0, sizeof(*hotload));
    hotload->manager = manager;
}

int digit_hotload_snapshot(
    digit_hotload_t *hotload
)
{
    size_t count;

    if (hotload == NULL)
    {
        return 0;
    }

    if (!digit_hotload_collect(hotload, hotload->files, &count))
    {
        return 0;
    }

    hotload->count = count;
    return 1;
}

int digit_hotload_poll(
    digit_hotload_t *hotload
)
{
    digit_hotload_file_t current[DIGIT_HOTLOAD_MAX_MODULES];
    size_t current_count;
    size_t index;
    int changes = 0;

    if (hotload == NULL || hotload->manager == NULL)
    {
        return -1;
    }

    if (!digit_hotload_collect(hotload, current, &current_count))
    {
        return -1;
    }

    for (index = 0; index < current_count; ++index)
    {
        const digit_hotload_file_t *previous = digit_hotload_find(
            hotload->files, hotload->count, current[index].module_id);

        if (previous == NULL)
        {
            printf("[MODULE] New candidate detected: %s\n", current[index].module_id);
            ++changes;
        }
        else if (previous->modified_time != current[index].modified_time ||
                 previous->size != current[index].size)
        {
            printf("[MODULE] Update candidate detected: %s\n", current[index].module_id);
            ++changes;
        }
    }

    memcpy(hotload->files, current, current_count * sizeof(current[0]));
    hotload->count = current_count;
    return changes;
}
