#ifndef DIGIT_CHANNEL_H
#define DIGIT_CHANNEL_H

#include <stddef.h>

#define DIGIT_CHANNEL_ID_MAX 64
#define DIGIT_CHANNEL_NAME_MAX 128
#define DIGIT_CHANNEL_ORIGIN_MAX 32
#define DIGIT_CHANNEL_MESSAGE_MAX 4096
#define DIGIT_CHANNEL_MAX 128
#define DIGIT_CHANNEL_MESSAGES_MAX 2048

typedef struct
{
    char id[DIGIT_CHANNEL_ID_MAX];
    char name[DIGIT_CHANNEL_NAME_MAX];
    unsigned long long created_at;
    unsigned long long last_activity_at;
    int active;
} digit_channel_t;

typedef struct
{
    char id[DIGIT_CHANNEL_ID_MAX];
    char channel_id[DIGIT_CHANNEL_ID_MAX];
    unsigned long long created_at;
    char origin[DIGIT_CHANNEL_ORIGIN_MAX];
    char body[DIGIT_CHANNEL_MESSAGE_MAX];
} digit_channel_message_t;

int digit_channel_init(void);
int digit_channel_create(const char *name, digit_channel_t *channel);
size_t digit_channel_list(digit_channel_t *channels, size_t capacity);
int digit_channel_get(const char *channel_id, digit_channel_t *channel);
int digit_channel_message_append(const char *channel_id, const char *origin, const char *body, digit_channel_message_t *message);
size_t digit_channel_message_list(const char *channel_id, digit_channel_message_t *messages, size_t capacity);

#endif
