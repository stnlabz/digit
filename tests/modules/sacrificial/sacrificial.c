#include <stddef.h>
#include <unistd.h>

#include "module.h"

/* [AI:GPT-6 | 2026-10-08] Versioned failure-injection fixture. */
#ifndef SACRIFICIAL_VERSION
#define SACRIFICIAL_VERSION 0
#endif
#ifndef SACRIFICIAL_FAIL_START
#define SACRIFICIAL_FAIL_START 0
#endif
#ifndef SACRIFICIAL_HANG_QUALIFY
#define SACRIFICIAL_HANG_QUALIFY 0
#endif
#ifndef SACRIFICIAL_FAIL_STOP
#define SACRIFICIAL_FAIL_STOP 0
#endif
#ifndef SACRIFICIAL_FAIL_RESTART
#define SACRIFICIAL_FAIL_RESTART 0
#endif
static unsigned int sacrificial_starts;

static stnlabz_module_result_t sacrificial_qualify(
    stnlabz_module_qualification_result_t *result
)
{
    if (result == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    if (SACRIFICIAL_HANG_QUALIFY) { for (;;) sleep(60); }
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->tests_failed = 0;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;

    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t sacrificial_start(
    const stnlabz_module_host_t *host
)
{
    if (host != NULL && host->send_message != NULL)
    {
        (void)host->send_message("[SACRIFICIAL] start");
    }

    ++sacrificial_starts;
    return (SACRIFICIAL_FAIL_START || (SACRIFICIAL_FAIL_RESTART && sacrificial_starts > 1)) ?
        STNLABZ_MODULE_ERR_START_FAILED : STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t sacrificial_stop(void)
{
    return SACRIFICIAL_FAIL_STOP ? STNLABZ_MODULE_ERR_STOP_FAILED : STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t sacrificial_descriptor =
{
    "sacrificial",
    "Digit Sacrificial Integration Module",
    1,
    0,
    SACRIFICIAL_VERSION,
    STNLABZ_MODULE_API_MAJOR,
    STNLABZ_MODULE_API_MINOR,
    sacrificial_qualify,
    sacrificial_start,
    sacrificial_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &sacrificial_descriptor;
}
