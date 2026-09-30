#include <stdio.h>
#include <string.h>

#include "abi.h"
#include "module_manager.h"

void digit_module_manager_init(
    digit_module_manager_t *manager,
    const stnlabz_module_host_t *host
)
{
    if (manager == NULL)
    {
        return;
    }

    memset(manager, 0, sizeof(*manager));
    stnlabz_module_registry_init(&manager->registry);
    stnlabz_module_loader_init(&manager->loader);

    if (host != NULL)
    {
        manager->host = *host;
    }
}

stnlabz_module_result_t digit_module_manager_discover(
    digit_module_manager_t *manager,
    stnlabz_module_discovery_report_t *report
)
{
    stnlabz_module_result_t result;

    if (manager == NULL || report == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    result = stnlabz_module_discovery_get_path(
        manager->modules_path,
        sizeof(manager->modules_path)
    );

    if (result != STNLABZ_MODULE_OK)
    {
        return result;
    }

    return stnlabz_module_discovery_scan(
        &manager->registry,
        &manager->loader,
        manager->modules_path,
        report
    );
}

stnlabz_module_result_t digit_module_manager_qualify_and_activate(
    digit_module_manager_t *manager
)
{
    size_t index;

    if (manager == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    for (index = 0; index < manager->registry.count; ++index)
    {
        stnlabz_module_record_t *record = &manager->registry.modules[index];
        stnlabz_module_result_t result;

        result = stnlabz_module_abi_prepare(
            &manager->registry,
            &record->descriptor
        );

        if (result != STNLABZ_MODULE_OK)
        {
            fprintf(stderr, "[MODULE] Qualification failed: %s (%s)\n",
                    record->descriptor.id,
                    stnlabz_module_result_string(result));
            return result;
        }

        record = &manager->registry.modules[index];

        if (record->state != STNLABZ_MODULE_STATE_QUALIFIED ||
            record->qualification.tests_executed < STNLABZ_MODULE_MIN_TESTS ||
            record->qualification.tests_passed != record->qualification.tests_executed ||
            record->qualification.tests_failed != 0 ||
            !record->qualification.negative_test_executed ||
            !record->qualification.negative_test_passed)
        {
            fprintf(stderr, "[MODULE] Qualification gate rejected: %s\n",
                    record->descriptor.id);
            return STNLABZ_MODULE_ERR_QUALIFICATION;
        }

        printf("[MODULE] Verification PASS: %s\n", record->descriptor.id);
        printf("[MODULE] Qualification PASS: %s (%u/%u)\n",
               record->descriptor.id,
               record->qualification.tests_passed,
               record->qualification.tests_executed);
        printf("[MODULE] Negative validation: PASS\n");

        result = stnlabz_module_abi_authorize_and_activate(
            &manager->registry,
            record->descriptor.id,
            &manager->host
        );

        if (result != STNLABZ_MODULE_OK)
        {
            fprintf(stderr, "[MODULE] Activation failed: %s (%s)\n",
                    record->descriptor.id,
                    stnlabz_module_result_string(result));
            return result;
        }

        printf("[MODULE] ACTIVE: %s\n", record->descriptor.id);
    }

    return STNLABZ_MODULE_OK;
}

void digit_module_manager_shutdown(
    digit_module_manager_t *manager
)
{
    size_t index;

    if (manager == NULL)
    {
        return;
    }

    for (index = manager->registry.count; index > 0; --index)
    {
        stnlabz_module_record_t *record = &manager->registry.modules[index - 1];

        if (record->state == STNLABZ_MODULE_STATE_ACTIVE)
        {
            (void)stnlabz_module_abi_stop(&manager->registry, record->descriptor.id);
        }
    }

    stnlabz_module_loader_unload_all(&manager->loader);
}

size_t digit_module_manager_count(
    const digit_module_manager_t *manager
)
{
    return manager == NULL ? 0 : manager->registry.count;
}

const char *digit_module_manager_path(
    const digit_module_manager_t *manager
)
{
    return manager == NULL ? NULL : manager->modules_path;
}
