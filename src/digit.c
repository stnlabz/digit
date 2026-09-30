#include <stdio.h>

#include "audit.h"
#include "digit.h"
#include "module.h"
#include "module_manager.h"
#include "runtime.h"

static digit_module_manager_t digit_modules;
static digit_runtime_t digit_runtime;

static int digit_send_message(const char *message)
{
    if (message == NULL)
    {
        return 0;
    }

    printf("%s\n", message);
    (void)digit_audit_event("MODULE", "MESSAGE", message);
    return 1;
}

static const stnlabz_module_host_t digit_host =
{
    digit_send_message,
    NULL,
    NULL
};

int digit_initialize(void)
{
    stnlabz_module_discovery_report_t report;
    stnlabz_module_result_t result;
    char detail[256];

    if (!digit_audit_open())
    {
        fprintf(stderr, "[CORE] Audit log unavailable: %s\n", DIGIT_AUDIT_PATH);
        return 1;
    }

    (void)digit_audit_event("CORE", "START", NULL);

    printf("%s\n", DIGIT_NAME);
    printf(
        "Digit is using the STN-LABZ ABI version %u.%u\n",
        STNLABZ_MODULE_API_MAJOR,
        STNLABZ_MODULE_API_MINOR
    );

    snprintf(detail, sizeof(detail), "abi=%u.%u",
             STNLABZ_MODULE_API_MAJOR,
             STNLABZ_MODULE_API_MINOR);
    (void)digit_audit_event("CORE", "ABI", detail);

    digit_module_manager_init(&digit_modules, &digit_host);

    result = digit_module_manager_discover(&digit_modules, &report);
    if (result != STNLABZ_MODULE_OK)
    {
        fprintf(stderr, "[MODULE] Discovery failed: %s\n",
                stnlabz_module_result_string(result));
        snprintf(detail, sizeof(detail), "result=%s",
                 stnlabz_module_result_string(result));
        (void)digit_audit_event("MODULE", "DISCOVERY_FAILED", detail);
        digit_module_manager_shutdown(&digit_modules);
        (void)digit_audit_event("CORE", "STOP", "reason=module_discovery_failure");
        digit_audit_close();
        return 1;
    }

    printf("[CORE] Module path: %s\n", digit_module_manager_path(&digit_modules));
    printf(
        "[MODULE] Discovery: %zu directories, %zu loaded, %zu discovered, %zu rejected\n",
        report.directories_examined,
        report.modules_loaded,
        report.modules_discovered,
        report.modules_rejected
    );

    snprintf(detail, sizeof(detail),
             "path=%s directories=%zu loaded=%zu discovered=%zu rejected=%zu",
             digit_module_manager_path(&digit_modules),
             report.directories_examined,
             report.modules_loaded,
             report.modules_discovered,
             report.modules_rejected);
    (void)digit_audit_event("MODULE", "DISCOVERY", detail);

    result = digit_module_manager_qualify_and_activate(&digit_modules);
    if (result != STNLABZ_MODULE_OK)
    {
        snprintf(detail, sizeof(detail), "result=%s",
                 stnlabz_module_result_string(result));
        (void)digit_audit_event("MODULE", "INITIALIZATION_FAILED", detail);
        digit_module_manager_shutdown(&digit_modules);
        (void)digit_audit_event("CORE", "STOP", "reason=module_initialization_failure");
        digit_audit_close();
        return 1;
    }

    digit_runtime_init(&digit_runtime, &digit_modules);

    printf("Core initialization: READY\n\n");
    printf("Greetings.\n\n");
    printf("What is today's mission?\n");
    printf("[CORE] Runtime: ACTIVE\n");
    (void)digit_audit_event("CORE", "READY", NULL);
    (void)digit_audit_event("CORE", "RUNTIME_ACTIVE", NULL);

    if (!digit_runtime_run(&digit_runtime))
    {
        fprintf(stderr, "[CORE] Runtime stopped unexpectedly.\n");
        (void)digit_audit_event("CORE", "RUNTIME_FAILED", NULL);
        digit_module_manager_shutdown(&digit_modules);
        (void)digit_audit_event("CORE", "STOP", "reason=runtime_failure");
        digit_audit_close();
        return 1;
    }

    printf("[CORE] Shutdown requested.\n");
    (void)digit_audit_event("CORE", "SHUTDOWN_REQUESTED", NULL);
    digit_module_manager_shutdown(&digit_modules);
    printf("[CORE] Shutdown complete.\n");
    (void)digit_audit_event("CORE", "STOP", "reason=clean_shutdown");
    digit_audit_close();
    return 0;
}
