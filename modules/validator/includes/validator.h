#ifndef DIGIT_VALIDATOR_H
#define DIGIT_VALIDATOR_H

#include "module.h"

#define DIGIT_VALIDATOR_INBOUND_SERVICE "validator.inbound"
#define DIGIT_VALIDATOR_OUTBOUND_SERVICE "validator.outbound"
#define DIGIT_VALIDATOR_TEXT_MAX 4096
#define DIGIT_VALIDATOR_REASON_MAX 256
#define DIGIT_VALIDATOR_MAX_ATTEMPTS 3

typedef enum {
    DIGIT_VALIDATOR_PASS = 0,
    DIGIT_VALIDATOR_FAIL = 1
} digit_validator_status_t;

typedef enum {
    DIGIT_VALIDATOR_CONFIDENCE_LOW = 0,
    DIGIT_VALIDATOR_CONFIDENCE_MODERATE = 1,
    DIGIT_VALIDATOR_CONFIDENCE_HIGH = 2
} digit_validator_confidence_t;

typedef struct {
    char raw[DIGIT_VALIDATOR_TEXT_MAX];
} digit_validator_inbound_request_t;

typedef struct {
    digit_validator_status_t status;
    digit_validator_confidence_t confidence;
    char normalized[DIGIT_VALIDATOR_TEXT_MAX];
    char reason[DIGIT_VALIDATOR_REASON_MAX];
} digit_validator_inbound_result_t;

typedef struct {
    char raw[DIGIT_VALIDATOR_TEXT_MAX];
    char normalized[DIGIT_VALIDATOR_TEXT_MAX];
    char candidate[DIGIT_VALIDATOR_TEXT_MAX];
    char evidence[DIGIT_VALIDATOR_TEXT_MAX];
    unsigned int attempt;
} digit_validator_outbound_request_t;

typedef struct {
    digit_validator_status_t status;
    int retry_allowed;
    char reason[DIGIT_VALIDATOR_REASON_MAX];
} digit_validator_outbound_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
