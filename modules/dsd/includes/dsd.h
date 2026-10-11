#ifndef DIGIT_DSD_H
#define DIGIT_DSD_H
#include <stddef.h>
#include "module.h"

/* [AI:GPT-6 | 2026-10-10] Read-only diagnostic contract.
 * The caller provides verified module-stage observations, never secrets
 * or raw conversation. DSD makes no authority or deployment decisions. */
#define DIGIT_DSD_SERVICE "dsd.diagnose"
#define DIGIT_DSD_ENGINEERING_SERVICE "dsd.diagnose_engineering"
#define DIGIT_DSD_NAME_MAX 48
#define DIGIT_DSD_CODE_MAX 64
#define DIGIT_DSD_REPORT_MAX 768
#define DIGIT_DSD_SYMPTOM_MAX 192
#define DIGIT_DSD_LOCATION_MAX 160
#define DIGIT_DSD_STAGE_COUNT 6

typedef enum {
    DIGIT_DSD_UNOBSERVED = 0,
    DIGIT_DSD_OK = 1,
    DIGIT_DSD_FAILED = 2,
    DIGIT_DSD_UNAVAILABLE = 3
} digit_dsd_stage_state_t;

typedef enum {
    DIGIT_DSD_UNKNOWN = 0,
    DIGIT_DSD_OBSERVED = 1,
    DIGIT_DSD_INFERRED = 2
} digit_dsd_finding_t;

typedef struct {
    char module[DIGIT_DSD_NAME_MAX];
    digit_dsd_stage_state_t state;
    char reason[DIGIT_DSD_CODE_MAX];
} digit_dsd_stage_t;

typedef struct {
    /* No account names, raw user messages, tokens, or personal history. */
    digit_dsd_stage_t stages[DIGIT_DSD_STAGE_COUNT];
    unsigned int stage_count;
    unsigned int retrieved;
    unsigned int selected;
    unsigned int candidate_constructed;
    unsigned int outbound_approved;
} digit_dsd_request_t;

typedef enum {
    DIGIT_DSD_LOCATION_UNRESOLVED = 0,
    DIGIT_DSD_LOCATION_INVESTIGATE = 1,
    DIGIT_DSD_LOCATION_CONFIRMED = 2
} digit_dsd_location_status_t;

typedef struct {
    digit_dsd_finding_t finding;
    char affected_module[DIGIT_DSD_NAME_MAX];
    char reason[DIGIT_DSD_CODE_MAX];
    char report[DIGIT_DSD_REPORT_MAX];
} digit_dsd_result_t;

/* The legacy result is a strict prefix: dsd.diagnose remains byte-compatible. */
typedef struct {
    digit_dsd_result_t diagnostic;
    char symptom[DIGIT_DSD_SYMPTOM_MAX];
    char source_location[DIGIT_DSD_LOCATION_MAX];
    digit_dsd_location_status_t location_status;
} digit_dsd_engineering_result_t;
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);
#endif
