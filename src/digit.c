#include <stdio.h>

#include "audit.h"
#include "digit.h"
#include "module.h"
#include "module_manager.h"
#include "runtime.h"
#include "service_registry.h"

static digit_module_manager_t digit_modules;
static digit_runtime_t digit_runtime;

static int digit_send_message(const char *message)
{
    if (message == NULL) return 0;
    printf("%s\n", message);
    (void)digit_audit_event("MODULE", "MESSAGE", message);
    return 1;
}

static const stnlabz_module_host_t digit_host =
{
    digit_send_message,
    NULL,
    NULL,
    digit_service_register,
    digit_service_unregister,
    digit_service_invoke
};

int digit_initialize(void)
{
    stnlabz_module_discovery_report_t report;
    stnlabz_module_result_t result;
    char detail[512];
    int degraded = 0;

    if (!digit_audit_open())
    {
        fprintf(stderr, "[CORE] Audit log unavailable: %s\n", DIGIT_AUDIT_PATH);
        return 1;
    }

    (void)digit_audit_event("CORE", "START", NULL);
    printf("%s\n", DIGIT_NAME);
    printf("Digit is using the STN-LABZ ABI version %u.%u\n", STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR);
    snprintf(detail, sizeof(detail), "abi=%u.%u", STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR);
    (void)digit_audit_event("CORE", "ABI", detail);

    digit_module_manager_init(&digit_modules, &digit_host);
    result = digit_module_manager_discover(&digit_modules, &report);
    if (result != STNLABZ_MODULE_OK)
    {
        fprintf(stderr, "[MODULE] Discovery subsystem failed: %s\n", stnlabz_module_result_string(result));
        snprintf(detail, sizeof(detail), "result=%s core_continuing=false", stnlabz_module_result_string(result));
        (void)digit_audit_event("MODULE", "DISCOVERY_FAILED", detail);
        digit_module_manager_shutdown(&digit_modules);
        (void)digit_audit_event("CORE", "STOP", "reason=module_discovery_subsystem_failure");
        digit_audit_close();
        return 1;
    }

    printf("[CORE] Module path: %s\n", digit_module_manager_path(&digit_modules));
    printf("[MODULE] Discovery: %zu directories, %zu loaded, %zu discovered, %zu rejected\n", report.directories_examined, report.modules_loaded, report.modules_discovered, report.modules_rejected);
    snprintf(detail, sizeof(detail), "path=%s directories=%zu loaded=%zu discovered=%zu rejected=%zu", digit_module_manager_path(&digit_modules), report.directories_examined, report.modules_loaded, report.modules_discovered, report.modules_rejected);
    (void)digit_audit_event("MODULE", "DISCOVERY", detail);

    if (report.modules_rejected > 0)
    {
        degraded = 1;
        snprintf(detail, sizeof(detail),
                 "directory=%s module=%s stage=%s reason=%s core_continuing=true",
                 report.rejected_directory[0] != '\0' ? report.rejected_directory : "unknown",
                 report.rejected_module[0] != '\0' ? report.rejected_module : "unknown",
                 report.rejection_stage[0] != '\0' ? report.rejection_stage : "unknown",
                 report.rejection_reason[0] != '\0' ? report.rejection_reason : "unknown");
        fprintf(stderr, "[CORE] DEGRADED: module rejection: %s\n", detail);
        (void)digit_audit_event("MODULE", "REJECTED", detail);

        snprintf(detail, sizeof(detail), "rejected=%zu core_continuing=true", report.modules_rejected);
        (void)digit_audit_event("CORE", "DEGRADED", detail);
    }

    result = digit_module_manager_qualify_and_activate(&digit_modules);
    if (result != STNLABZ_MODULE_OK)
    {
        degraded = 1;
        snprintf(detail, sizeof(detail), "result=%s core_continuing=true", stnlabz_module_result_string(result));
        fprintf(stderr, "[CORE] DEGRADED: module initialization reported %s; Digit Core is continuing.\n", stnlabz_module_result_string(result));
        (void)digit_audit_event("CORE", "DEGRADED", detail);
    }

    digit_runtime_init(&digit_runtime, &digit_modules);

    if (degraded)
    {
        printf("Core initialization: READY (DEGRADED)\n\nGreetings.\n\nDigit is operational with reduced module capability. Check the audit log for rejected or unavailable modules.\n[CORE] Runtime: ACTIVE\n");
        (void)digit_audit_event("CORE", "READY", "state=DEGRADED");
    }
    else
    {
        printf("Core initialization: READY\n\nGreetings.\n\nWhat is today's mission?\n[CORE] Runtime: ACTIVE\n");
        (void)digit_audit_event("CORE", "READY", "state=GREEN");
    }

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
