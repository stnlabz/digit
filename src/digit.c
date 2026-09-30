#include <stdio.h>

#include "digit.h"
#include "abi.h"
#include "module.h"
#include "module_discovery.h"
#include "module_loader.h"
#include "module_registry.h"

static stnlabz_module_registry_t digit_registry;
static stnlabz_module_loader_t digit_loader;

static int digit_send_message(const char *message)
{
    if (message == NULL)
    {
        return 0;
    }

    printf("%s\n", message);
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
    char modules_path[STNLABZ_MODULE_LOADER_PATH_MAX];
    stnlabz_module_discovery_report_t report;
    stnlabz_module_result_t result;
    size_t index;

    printf("%s\n", DIGIT_NAME);
    printf(
        "Digit is using the STN-LABZ ABI version %u.%u\n",
        STNLABZ_MODULE_API_MAJOR,
        STNLABZ_MODULE_API_MINOR
    );

    stnlabz_module_registry_init(&digit_registry);
    stnlabz_module_loader_init(&digit_loader);

    result = stnlabz_module_discovery_get_path(
        modules_path,
        sizeof(modules_path)
    );

    if (result != STNLABZ_MODULE_OK)
    {
        fprintf(
            stderr,
            "[CORE] Module path resolution failed: %s\n",
            stnlabz_module_result_string(result)
        );
        return 1;
    }

    printf("[CORE] Module path: %s\n", modules_path);

    result = stnlabz_module_discovery_scan(
        &digit_registry,
        &digit_loader,
        modules_path,
        &report
    );

    if (result != STNLABZ_MODULE_OK)
    {
        fprintf(
            stderr,
            "[MODULE] Discovery failed: %s\n",
            stnlabz_module_result_string(result)
        );
        stnlabz_module_loader_unload_all(&digit_loader);
        return 1;
    }

    printf(
        "[MODULE] Discovery: %zu directories, %zu loaded, %zu discovered, %zu rejected\n",
        report.directories_examined,
        report.modules_loaded,
        report.modules_discovered,
        report.modules_rejected
    );

    for (index = 0; index < digit_registry.count; ++index)
    {
        const stnlabz_module_record_t *record;

        record = &digit_registry.modules[index];

        printf(
            "[MODULE] Discovered: %s (%s)\n",
            record->descriptor.name,
            record->descriptor.id
        );

        if (record->state != STNLABZ_MODULE_STATE_QUALIFIED)
        {
            fprintf(
                stderr,
                "[MODULE] Qualification failed: %s (%s)\n",
                record->descriptor.id,
                stnlabz_module_state_string(record->state)
            );
            stnlabz_module_loader_unload_all(&digit_loader);
            return 1;
        }

        printf("[MODULE] Verification PASS: %s\n", record->descriptor.id);
        printf(
            "[MODULE] Qualification PASS: %s (%u/%u)\n",
            record->descriptor.id,
            record->qualification.tests_passed,
            record->qualification.tests_executed
        );
        printf(
            "[MODULE] Negative validation: %s\n",
            record->qualification.negative_test_passed ? "PASS" : "FAIL"
        );
        printf("[MODULE] Activation starting: %s\n", record->descriptor.id);

        result = stnlabz_module_abi_authorize_and_activate(
            &digit_registry,
            record->descriptor.id,
            &digit_host
        );

        if (result != STNLABZ_MODULE_OK)
        {
            fprintf(
                stderr,
                "[MODULE] Activation failed: %s (%s)\n",
                record->descriptor.id,
                stnlabz_module_result_string(result)
            );
            stnlabz_module_loader_unload_all(&digit_loader);
            return 1;
        }

        printf("[MODULE] ACTIVE: %s\n", record->descriptor.id);
    }

    printf("Core initialization: READY\n\n");
    printf("Greetings.\n\n");
    printf("What is today's mission?\n");

    return 0;
}
