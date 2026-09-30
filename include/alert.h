#ifndef DIGIT_ALERT_H
#define DIGIT_ALERT_H

#include <stddef.h>

#define DIGIT_ALERT_ID_MAX 64
#define DIGIT_ALERT_SOURCE_MAX 64
#define DIGIT_ALERT_SUMMARY_MAX 256
#define DIGIT_ALERT_DETAIL_MAX 1024
#define DIGIT_ALERT_STATE_MAX 32
#define DIGIT_ALERT_MAX 1024

typedef enum
{
    DIGIT_ALERT_INFO = 0,
    DIGIT_ALERT_WARNING = 1,
    DIGIT_ALERT_ERROR = 2,
    DIGIT_ALERT_CRITICAL = 3
} digit_alert_severity_t;

typedef struct
{
    char id[DIGIT_ALERT_ID_MAX];
    unsigned long long created_at;
    digit_alert_severity_t severity;
    char source[DIGIT_ALERT_SOURCE_MAX];
    char summary[DIGIT_ALERT_SUMMARY_MAX];
    char detail[DIGIT_ALERT_DETAIL_MAX];
    char operational_state[DIGIT_ALERT_STATE_MAX];
    int acknowledged;
    unsigned long long acknowledged_at;
} digit_alert_t;

int digit_alert_init(void);
int digit_alert_raise(digit_alert_severity_t severity, const char *source, const char *summary,
                      const char *detail, const char *operational_state, digit_alert_t *alert);
size_t digit_alert_list(digit_alert_t *alerts, size_t capacity, int unacknowledged_only);
int digit_alert_get(const char *alert_id, digit_alert_t *alert);
int digit_alert_acknowledge(const char *alert_id, digit_alert_t *alert);
const char *digit_alert_severity_string(digit_alert_severity_t severity);

#endif
