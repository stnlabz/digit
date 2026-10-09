#include <stdio.h>
#include <string.h>

#include "abi.h"
#include "authority.h"
#include "module_manager.h"
#include "qualification_store.h"
#include "audit.h"

#define DIGIT_RUNTIME_MODULE_PATH "/opt/digit/modules"

static int digit_module_manager_set_qualification_path(
    digit_module_manager_t *manager
)
{
    int written;

    written = snprintf(manager->qualification_path,
                       sizeof(manager->qualification_path),
                       "%s/%s",
                       manager->modules_path,
                       DIGIT_QUALIFICATION_STATE_FILE);

    return written >= 0 && (size_t)written < sizeof(manager->qualification_path);
}

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
    digit_qualification_init(&manager->qualifications);

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
    if (manager == NULL || report == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    if (snprintf(manager->modules_path,
                 sizeof(manager->modules_path),
                 "%s",
                 DIGIT_RUNTIME_MODULE_PATH) >= (int)sizeof(manager->modules_path))
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    if (!digit_module_manager_set_qualification_path(manager))
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    if (!digit_qualification_store_load(manager->qualification_path,
                                        &manager->qualifications))
    {
        fprintf(stderr, "[MODULE] Qualification state rejected: %s\n",
                manager->qualification_path);
        return STNLABZ_MODULE_ERR_QUALIFICATION;
    }

    return stnlabz_module_discovery_scan(&manager->registry,
                                         &manager->loader,
                                         manager->modules_path,
                                         report);
}

stnlabz_module_result_t digit_module_manager_qualify_and_activate(
    digit_module_manager_t *manager
)
{
    size_t index;
    unsigned int rejected = 0;

    if (manager == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    for (index = 0; index < manager->registry.count; ++index)
    {
        stnlabz_module_record_t *record = &manager->registry.modules[index];
        stnlabz_module_result_t result;

        if (digit_authority_requires_qualification(&manager->qualifications,
                                                   &record->descriptor))
        {
            result = stnlabz_module_registry_verify(&manager->registry,
                                                    record->descriptor.id);
            if (result != STNLABZ_MODULE_OK)
            {
                fprintf(stderr, "[MODULE] Verification failed: %s (%s)\n",
                        record->descriptor.id,
                        stnlabz_module_result_string(result));
                fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                        record->descriptor.id);
                ++rejected;
            continue;
            }

            result = stnlabz_module_registry_qualify(&manager->registry,
                                                     record->descriptor.id);
            if (result != STNLABZ_MODULE_OK)
            {
                fprintf(stderr, "[MODULE] Qualification failed: %s (%s)\n",
                        record->descriptor.id,
                        stnlabz_module_result_string(result));
                fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                        record->descriptor.id);
                ++rejected;
            continue;
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
                fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                        record->descriptor.id);
                ++rejected;
            continue;
            }

            if (!digit_qualification_record(&manager->qualifications,
                                            &record->descriptor) ||
                !digit_qualification_store_save(manager->qualification_path,
                                                &manager->qualifications))
            {
                fprintf(stderr, "[MODULE] Qualification persistence failed: %s\n",
                        record->descriptor.id);
                fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                        record->descriptor.id);
                ++rejected;
            continue;
            }

            printf("[MODULE] Verification PASS: %s\n", record->descriptor.id);
            printf("[MODULE] Qualification PASS: %s (%u/%u)\n",
                   record->descriptor.id,
                   record->qualification.tests_passed,
                   record->qualification.tests_executed);
            printf("[MODULE] Negative validation: PASS\n");
        }
        else
        {
            stnlabz_module_qualification_result_t restored;

            memset(&restored, 0, sizeof(restored));
            restored.tests_executed = STNLABZ_MODULE_MIN_TESTS;
            restored.tests_passed = STNLABZ_MODULE_MIN_TESTS;
            restored.negative_test_executed = 1;
            restored.negative_test_passed = 1;

            result = stnlabz_module_registry_verify(&manager->registry,
                                                    record->descriptor.id);
            if (result != STNLABZ_MODULE_OK)
            {
                fprintf(stderr, "[MODULE] Verification failed: %s (%s)\n",
                        record->descriptor.id,
                        stnlabz_module_result_string(result));
                fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                        record->descriptor.id);
                ++rejected;
            continue;
            }

            result = stnlabz_module_registry_restore_qualification(
                &manager->registry,
                record->descriptor.id,
                &restored);
            if (result != STNLABZ_MODULE_OK)
            {
                fprintf(stderr, "[MODULE] Qualification restore failed: %s (%s)\n",
                        record->descriptor.id,
                        stnlabz_module_result_string(result));
                fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                        record->descriptor.id);
                ++rejected;
            continue;
            }

            printf("[MODULE] Qualification restored: %s %u.%u.%u\n",
                   record->descriptor.id,
                   record->descriptor.version_major,
                   record->descriptor.version_minor,
                   record->descriptor.version_patch);
        }

        record = &manager->registry.modules[index];
        result = stnlabz_module_registry_authorize_activation(&manager->registry,
                                                              record->descriptor.id);
        if (result != STNLABZ_MODULE_OK)
        {
            fprintf(stderr, "[MODULE] Activation authorization failed: %s (%s)\n",
                    record->descriptor.id,
                    stnlabz_module_result_string(result));
            fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                    record->descriptor.id);
            ++rejected;
            continue;
        }

        record = &manager->registry.modules[index];
        if (digit_authority_may_activate(record) != DIGIT_AUTHORITY_ALLOW)
        {
            fprintf(stderr, "[MODULE] Authority denied activation: %s\n",
                    record->descriptor.id);
            fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                    record->descriptor.id);
            ++rejected;
            continue;
        }

        result = stnlabz_module_registry_activate(&manager->registry,
                                                  record->descriptor.id);
        if (result != STNLABZ_MODULE_OK)
        {
            fprintf(stderr, "[MODULE] Activation failed: %s (%s)\n",
                    record->descriptor.id,
                    stnlabz_module_result_string(result));
            fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                    record->descriptor.id);
            ++rejected;
            continue;
        }

        if (record->descriptor.start != NULL &&
            record->descriptor.start(&manager->host) != STNLABZ_MODULE_OK)
        {
            (void)stnlabz_module_registry_fail(&manager->registry,
                                               record->descriptor.id);
            digit_audit_lifecycle(record->descriptor.id, "START_FAILED", "source=startup");
            fprintf(stderr, "[MODULE] Start failed: %s\n",
                    record->descriptor.id);
            fprintf(stderr, "[MODULE] REJECTED: %s -- Core continuing\n",
                    record->descriptor.id);
            ++rejected;
            continue;
        }

        digit_audit_lifecycle(record->descriptor.id, "ACTIVATED", "source=startup");
        printf("[MODULE] ACTIVE: %s\n", record->descriptor.id);
    }

    return rejected == 0 ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_START_FAILED;
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
            (void)stnlabz_module_abi_stop(&manager->registry,
                                          record->descriptor.id);
        }
    }

    stnlabz_module_loader_unload_all(&manager->loader);
}

size_t digit_module_manager_count(const digit_module_manager_t *manager)
{
    return manager == NULL ? 0 : manager->registry.count;
}

const char *digit_module_manager_path(const digit_module_manager_t *manager)
{
    return manager == NULL ? NULL : manager->modules_path;
}
