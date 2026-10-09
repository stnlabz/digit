#ifndef DIGIT_INTERFACE_ALERT_RESULTS_H
#define DIGIT_INTERFACE_ALERT_RESULTS_H
#include <stddef.h>
#include "alert.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.8: bounded Core alert-list
 * admission before JSON serialization. */
int digit_interface_alerts_valid(const digit_alert_t *alerts,size_t count,
                                 size_t capacity,int unacknowledged_only);
/* [AI:GPT-6 | 2026-10-08] 1.4.9: exact get/ack response identity. */
int digit_interface_alert_exact_valid(const digit_alert_t *alert,int present,
                                      const char *requested_id,int require_ack);
#endif
