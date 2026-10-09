#include <string.h>
#include "builder_response.h"

/* [AI:GPT-6 | 2026-10-08] 1.5.3: fail closed on malformed
 * Corpus Builder results before constructing an HTTP reply. */
static int terminated(const char *field, size_t capacity)
{
    return memchr(field, '\0', capacity) != NULL;
}

static int record_id_valid(const char *id)
{
    size_t i;
    if (strncmp(id, "DIGIT-", 6) != 0 || strlen(id) != 22)
        return 0;
    for (i = 6; i < 22; ++i)
    {
        if (!((id[i] >= '0' && id[i] <= '9') ||
              (id[i] >= 'a' && id[i] <= 'f')))
            return 0;
    }
    return 1;
}

int digit_interface_builder_result_valid(
    const digit_interface_builder_result_t *result)
{
    if (result == NULL ||
        (result->candidate != 0 && result->candidate != 1) ||
        (result->stored != 0 && result->stored != 1) ||
        result->confidence > 100U ||
        (result->stored && !result->candidate) ||
        !terminated(result->category, sizeof(result->category)) ||
        !terminated(result->record_id, sizeof(result->record_id)) ||
        !terminated(result->reason, sizeof(result->reason)) ||
        result->category[0] == '\0' || result->reason[0] == '\0')
        return 0;

    if (result->candidate)
        return record_id_valid(result->record_id);
    return result->record_id[0] == '\0';
}
