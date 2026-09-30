#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "channel.h"

#define DIGIT_CHANNEL_STATE_DIR "/opt/digit/state/channels"
#define DIGIT_CHANNELS_FILE DIGIT_CHANNEL_STATE_DIR "/channels.tsv"
#define DIGIT_MESSAGES_FILE DIGIT_CHANNEL_STATE_DIR "/messages.tsv"

static unsigned long long digit_channel_sequence = 0;

static unsigned long long digit_channel_now(void)
{
    return (unsigned long long)time(NULL);
}

static int digit_channel_safe_text(const char *text)
{
    if (text == NULL || text[0] == '\0') return 0;
    return strchr(text, '\t') == NULL && strchr(text, '\n') == NULL && strchr(text, '\r') == NULL;
}

static void digit_channel_make_id(char *output, size_t size, const char *prefix)
{
    unsigned long long now = digit_channel_now();
    ++digit_channel_sequence;
    (void)snprintf(output, size, "%s-%llu-%llu", prefix, now, digit_channel_sequence);
}

static int digit_channel_ensure_dir(void)
{
    if (mkdir(DIGIT_CHANNEL_STATE_DIR, 0750) == 0 || errno == EEXIST) return 1;
    return 0;
}

static int digit_channel_write(const digit_channel_t *channel)
{
    FILE *file;
    if (!digit_channel_ensure_dir()) return 0;
    file = fopen(DIGIT_CHANNELS_FILE, "a");
    if (file == NULL) return 0;
    if (fprintf(file, "%s\t%s\t%llu\t%llu\t%d\n", channel->id, channel->name,
                channel->created_at, channel->last_activity_at, channel->active) < 0)
    {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

int digit_channel_init(void)
{
    digit_channel_t channel;
    if (!digit_channel_ensure_dir()) return 0;
    if (digit_channel_get("general", &channel)) return 1;
    memset(&channel, 0, sizeof(channel));
    (void)snprintf(channel.id, sizeof(channel.id), "general");
    (void)snprintf(channel.name, sizeof(channel.name), "General");
    channel.created_at = digit_channel_now();
    channel.last_activity_at = channel.created_at;
    channel.active = 1;
    return digit_channel_write(&channel);
}

int digit_channel_create(const char *name, digit_channel_t *channel)
{
    digit_channel_t created;
    if (!digit_channel_safe_text(name) || strlen(name) >= sizeof(created.name)) return 0;
    memset(&created, 0, sizeof(created));
    digit_channel_make_id(created.id, sizeof(created.id), "channel");
    (void)snprintf(created.name, sizeof(created.name), "%s", name);
    created.created_at = digit_channel_now();
    created.last_activity_at = created.created_at;
    created.active = 1;
    if (!digit_channel_write(&created)) return 0;
    if (channel != NULL) *channel = created;
    return 1;
}

size_t digit_channel_list(digit_channel_t *channels, size_t capacity)
{
    FILE *file;char line[4608];size_t count = 0;
    file = fopen(DIGIT_CHANNELS_FILE, "r");if (file == NULL) return 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        digit_channel_t item;int active;
        memset(&item, 0, sizeof(item));
        if (sscanf(line, "%63[^\t]\t%127[^\t]\t%llu\t%llu\t%d", item.id, item.name,
                   &item.created_at, &item.last_activity_at, &active) != 5) continue;
        item.active = active;
        if (channels != NULL && count < capacity) channels[count] = item;
        ++count;
    }
    fclose(file);
    return count;
}

int digit_channel_get(const char *channel_id, digit_channel_t *channel)
{
    digit_channel_t items[DIGIT_CHANNEL_MAX];size_t count;size_t index;
    if (channel_id == NULL || channel == NULL) return 0;
    count = digit_channel_list(items, DIGIT_CHANNEL_MAX);
    if (count > DIGIT_CHANNEL_MAX) count = DIGIT_CHANNEL_MAX;
    for (index = 0; index < count; ++index)
    {
        if (strcmp(items[index].id, channel_id) == 0) {*channel = items[index];return 1;}
    }
    return 0;
}

int digit_channel_message_append(const char *channel_id, const char *origin, const char *body, digit_channel_message_t *message)
{
    FILE *file;digit_channel_t channel;digit_channel_message_t created;
    if (!digit_channel_get(channel_id, &channel) || !digit_channel_safe_text(origin) ||
        !digit_channel_safe_text(body) || strlen(origin) >= sizeof(created.origin) ||
        strlen(body) >= sizeof(created.body)) return 0;
    if (!digit_channel_ensure_dir()) return 0;
    memset(&created, 0, sizeof(created));
    digit_channel_make_id(created.id, sizeof(created.id), "message");
    (void)snprintf(created.channel_id, sizeof(created.channel_id), "%s", channel_id);
    created.created_at = digit_channel_now();
    (void)snprintf(created.origin, sizeof(created.origin), "%s", origin);
    (void)snprintf(created.body, sizeof(created.body), "%s", body);
    file = fopen(DIGIT_MESSAGES_FILE, "a");if (file == NULL) return 0;
    if (fprintf(file, "%s\t%s\t%llu\t%s\t%s\n", created.id, created.channel_id,
                created.created_at, created.origin, created.body) < 0)
    {fclose(file);return 0;}
    if (fclose(file) != 0) return 0;
    if (message != NULL) *message = created;
    return 1;
}

size_t digit_channel_message_list(const char *channel_id, digit_channel_message_t *messages, size_t capacity)
{
    FILE *file;char line[4608];size_t count = 0;
    if (channel_id == NULL) return 0;
    file = fopen(DIGIT_MESSAGES_FILE, "r");if (file == NULL) return 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        digit_channel_message_t item;
        memset(&item, 0, sizeof(item));
        if (sscanf(line, "%63[^\t]\t%63[^\t]\t%llu\t%31[^\t]\t%4095[^\n]", item.id,
                   item.channel_id, &item.created_at, item.origin, item.body) != 5) continue;
        if (strcmp(item.channel_id, channel_id) != 0) continue;
        if (messages != NULL && count < capacity) messages[count] = item;
        ++count;
    }
    fclose(file);
    return count;
}
