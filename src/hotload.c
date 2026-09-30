#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hotload.h"
#include "qualification_store.h"

#define DIGIT_CANDIDATE_SUFFIX ".candidate.XXXXXX"

typedef struct
{
    int valid;
    char module_id[STNLABZ_MODULE_ID_MAX];
    unsigned int version_major;
    unsigned int version_minor;
    unsigned int version_patch;
    stnlabz_module_qualification_result_t qualification;
} digit_candidate_result_t;

static int digit_hotload_collect(digit_hotload_t *hotload, digit_hotload_file_t *files, size_t *count)
{
    DIR *root;
    struct dirent *entry;
    if (hotload == NULL || hotload->manager == NULL || files == NULL || count == NULL) return 0;
    *count = 0;
    root = opendir(digit_module_manager_path(hotload->manager));
    if (root == NULL) return 0;
    while ((entry = readdir(root)) != NULL)
    {
        char path[DIGIT_HOTLOAD_PATH_MAX];
        char so_path[DIGIT_HOTLOAD_PATH_MAX];
        struct stat status;
        size_t module_id_length;
        int written;
        if (entry->d_name[0] == '.') continue;
        module_id_length = strlen(entry->d_name);
        if (module_id_length == 0 || module_id_length >= STNLABZ_MODULE_ID_MAX) continue;
        written = snprintf(path, sizeof(path), "%s/%s", digit_module_manager_path(hotload->manager), entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(path) || stat(path, &status) != 0 || !S_ISDIR(status.st_mode)) continue;
        written = snprintf(so_path, sizeof(so_path), "%s/%s.so", path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(so_path) || stat(so_path, &status) != 0 || !S_ISREG(status.st_mode)) continue;
        if (*count >= DIGIT_HOTLOAD_MAX_MODULES) { closedir(root); return 0; }
        memcpy(files[*count].module_id, entry->d_name, module_id_length + 1);
        snprintf(files[*count].path, sizeof(files[*count].path), "%s", so_path);
        files[*count].modified_time = status.st_mtime;
        files[*count].size = status.st_size;
        ++(*count);
    }
    closedir(root);
    return 1;
}

static const digit_hotload_file_t *digit_hotload_find(const digit_hotload_file_t *files, size_t count, const char *module_id)
{
    size_t index;
    for (index = 0; index < count; ++index) if (strcmp(files[index].module_id, module_id) == 0) return &files[index];
    return NULL;
}

static int digit_write_all(int fd, const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t offset = 0;
    while (offset < size)
    {
        ssize_t sent = write(fd, bytes + offset, size - offset);
        if (sent <= 0) return 0;
        offset += (size_t)sent;
    }
    return 1;
}

static int digit_read_all(int fd, void *data, size_t size)
{
    unsigned char *bytes = data;
    size_t offset = 0;
    while (offset < size)
    {
        ssize_t received = read(fd, bytes + offset, size - offset);
        if (received <= 0) return 0;
        offset += (size_t)received;
    }
    return 1;
}

static int digit_copy_candidate(const char *source, char *staged, size_t staged_size)
{
    int source_fd;
    int staged_fd;
    char buffer[16384];
    ssize_t received;
    int written;
    written = snprintf(staged, staged_size, "%s%s", source, DIGIT_CANDIDATE_SUFFIX);
    if (written < 0 || (size_t)written >= staged_size) return 0;
    staged_fd = mkstemp(staged);
    if (staged_fd < 0) return 0;
    source_fd = open(source, O_RDONLY);
    if (source_fd < 0) { close(staged_fd); unlink(staged); return 0; }
    while ((received = read(source_fd, buffer, sizeof(buffer))) > 0)
    {
        if (!digit_write_all(staged_fd, buffer, (size_t)received)) { close(source_fd); close(staged_fd); unlink(staged); return 0; }
    }
    close(source_fd);
    if (close(staged_fd) != 0 || received < 0) { unlink(staged); return 0; }
    return 1;
}

static int digit_qualify_candidate_child(const char *module_id, const char *path, digit_candidate_result_t *result)
{
    void *handle;
    void *symbol;
    stnlabz_module_get_descriptor_fn get_descriptor;
    const stnlabz_module_descriptor_t *descriptor;
    stnlabz_module_result_t qualification_result;
    memset(result, 0, sizeof(*result));
    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) return 0;
    symbol = dlsym(handle, STNLABZ_MODULE_DESCRIPTOR_EXPORT);
    if (symbol == NULL) { dlclose(handle); return 0; }
    memcpy(&get_descriptor, &symbol, sizeof(get_descriptor));
    descriptor = get_descriptor();
    if (descriptor == NULL || strcmp(descriptor->id, module_id) != 0 || descriptor->qualify == NULL) { dlclose(handle); return 0; }
    snprintf(result->module_id, sizeof(result->module_id), "%s", descriptor->id);
    result->version_major = descriptor->version_major;
    result->version_minor = descriptor->version_minor;
    result->version_patch = descriptor->version_patch;
    qualification_result = descriptor->qualify(&result->qualification);
    result->valid = qualification_result == STNLABZ_MODULE_OK &&
                    result->qualification.tests_executed >= STNLABZ_MODULE_MIN_TESTS &&
                    result->qualification.tests_passed == result->qualification.tests_executed &&
                    result->qualification.tests_failed == 0 &&
                    result->qualification.negative_test_executed &&
                    result->qualification.negative_test_passed;
    dlclose(handle);
    return 1;
}

static int digit_qualify_candidate_isolated(const char *module_id, const char *path, digit_candidate_result_t *result)
{
    int pipe_fd[2];
    pid_t child;
    int status;
    int received;
    if (pipe(pipe_fd) != 0) return 0;
    child = fork();
    if (child < 0) { close(pipe_fd[0]); close(pipe_fd[1]); return 0; }
    if (child == 0)
    {
        digit_candidate_result_t child_result;
        int inspected;
        int sent;
        close(pipe_fd[0]);
        inspected = digit_qualify_candidate_child(module_id, path, &child_result);
        sent = inspected && digit_write_all(pipe_fd[1], &child_result, sizeof(child_result));
        close(pipe_fd[1]);
        _exit(sent ? 0 : 1);
    }
    close(pipe_fd[1]);
    received = digit_read_all(pipe_fd[0], result, sizeof(*result));
    close(pipe_fd[0]);
    if (waitpid(child, &status, 0) != child) return 0;
    return received && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static void digit_report_red_candidate(digit_module_manager_t *manager, const digit_candidate_result_t *result)
{
    const stnlabz_module_record_t *active;
    printf("[MODULE] Qualification: %s %u.%u.%u\n", result->module_id, result->version_major, result->version_minor, result->version_patch);
    printf("[MODULE] Tests: %u/%u PASS, %u FAILED\n", result->qualification.tests_passed, result->qualification.tests_executed, result->qualification.tests_failed);
    printf("[MODULE] Negative validation: %s\n", result->qualification.negative_test_executed && result->qualification.negative_test_passed ? "PASS" : "FAIL");
    printf("[MODULE] Qualification RED: %s %u.%u.%u -- candidate rejected\n", result->module_id, result->version_major, result->version_minor, result->version_patch);
    active = stnlabz_module_registry_find(&manager->registry, result->module_id);
    if (active != NULL && active->state == STNLABZ_MODULE_STATE_ACTIVE)
    {
        printf("[MODULE] Incumbent retained: %s %u.%u.%u\n", active->descriptor.id, active->descriptor.version_major, active->descriptor.version_minor, active->descriptor.version_patch);
    }
}

static int digit_hotload_promote(digit_hotload_t *hotload, const digit_hotload_file_t *candidate)
{
    digit_module_manager_t *manager = hotload->manager;
    const stnlabz_module_record_t *active;
    digit_candidate_result_t result;
    stnlabz_module_descriptor_t identity;
    const stnlabz_module_descriptor_t *loaded_descriptor = NULL;
    stnlabz_module_loader_result_t loader_result;
    stnlabz_module_result_t module_result;
    char staged[DIGIT_HOTLOAD_PATH_MAX];
    int already_qualified;
    memset(&result, 0, sizeof(result));
    if (!digit_copy_candidate(candidate->path, staged, sizeof(staged))) { printf("[MODULE] Candidate staging failed: %s\n", candidate->module_id); return 0; }
    if (!digit_qualify_candidate_isolated(candidate->module_id, staged, &result))
    {
        printf("[MODULE] Candidate inspection failed: %s -- candidate rejected\n", candidate->module_id);
        unlink(staged);
        return 0;
    }
    if (!result.valid)
    {
        digit_report_red_candidate(manager, &result);
        unlink(staged);
        return 0;
    }
    memset(&identity, 0, sizeof(identity));
    snprintf(identity.id, sizeof(identity.id), "%s", result.module_id);
    identity.version_major = result.version_major;
    identity.version_minor = result.version_minor;
    identity.version_patch = result.version_patch;
    already_qualified = digit_qualification_contains(&manager->qualifications, &identity);
    active = stnlabz_module_registry_find(&manager->registry, candidate->module_id);
    if (active != NULL && active->descriptor.version_major == result.version_major && active->descriptor.version_minor == result.version_minor && active->descriptor.version_patch == result.version_patch)
    {
        printf("[MODULE] Candidate version unchanged: %s %u.%u.%u -- no requalification required\n", candidate->module_id, result.version_major, result.version_minor, result.version_patch);
        unlink(staged);
        return 1;
    }
    printf("[MODULE] Internal version changed: %s -> %u.%u.%u\n", candidate->module_id, result.version_major, result.version_minor, result.version_patch);
    if (!already_qualified)
    {
        printf("[MODULE] Qualification PASS: %s (%u/%u)\n", candidate->module_id, result.qualification.tests_passed, result.qualification.tests_executed);
        printf("[MODULE] Negative validation: PASS\n");
    }
    else printf("[MODULE] Qualification restored: %s %u.%u.%u\n", candidate->module_id, result.version_major, result.version_minor, result.version_patch);
    printf("[MODULE] Qualification GREEN: %s %u.%u.%u\n", candidate->module_id, result.version_major, result.version_minor, result.version_patch);
    active = stnlabz_module_registry_find(&manager->registry, candidate->module_id);
    if (active != NULL && active->state == STNLABZ_MODULE_STATE_ACTIVE)
    {
        if (active->descriptor.stop != NULL && active->descriptor.stop() != STNLABZ_MODULE_OK) { printf("[MODULE] Incumbent stop failed: %s\n", candidate->module_id); unlink(staged); return 0; }
        module_result = stnlabz_module_registry_stop(&manager->registry, candidate->module_id);
        if (module_result != STNLABZ_MODULE_OK) { unlink(staged); return 0; }
    }
    (void)stnlabz_module_loader_unload(&manager->loader, candidate->module_id);
    module_result = stnlabz_module_registry_unregister(&manager->registry, candidate->module_id);
    if (module_result != STNLABZ_MODULE_OK) { unlink(staged); return 0; }
    loader_result = stnlabz_module_loader_load(&manager->loader, candidate->module_id, staged, &loaded_descriptor);
    if (loader_result != STNLABZ_MODULE_LOADER_OK || loaded_descriptor == NULL) { printf("[MODULE] GREEN candidate load failed: %s\n", candidate->module_id); unlink(staged); return 0; }
    module_result = stnlabz_module_registry_discover(&manager->registry, loaded_descriptor);
    if (module_result == STNLABZ_MODULE_OK) module_result = stnlabz_module_registry_verify(&manager->registry, candidate->module_id);
    if (module_result == STNLABZ_MODULE_OK) module_result = stnlabz_module_registry_restore_qualification(&manager->registry, candidate->module_id, &result.qualification);
    if (module_result != STNLABZ_MODULE_OK) { printf("[MODULE] GREEN candidate registry admission failed: %s\n", candidate->module_id); unlink(staged); return 0; }
    if (!already_qualified && (!digit_qualification_record(&manager->qualifications, loaded_descriptor) || !digit_qualification_store_save(manager->qualification_path, &manager->qualifications))) { printf("[MODULE] Qualification persistence failed: %s\n", candidate->module_id); unlink(staged); return 0; }
    module_result = stnlabz_module_registry_authorize_activation(&manager->registry, candidate->module_id);
    if (module_result == STNLABZ_MODULE_OK) module_result = stnlabz_module_registry_activate(&manager->registry, candidate->module_id);
    if (module_result != STNLABZ_MODULE_OK) { printf("[MODULE] GREEN candidate activation denied: %s\n", candidate->module_id); unlink(staged); return 0; }
    if (loaded_descriptor->start != NULL && loaded_descriptor->start(&manager->host) != STNLABZ_MODULE_OK) { (void)stnlabz_module_registry_fail(&manager->registry, candidate->module_id); printf("[MODULE] GREEN candidate start failed: %s\n", candidate->module_id); unlink(staged); return 0; }
    unlink(staged);
    printf("[MODULE] HOTLOAD ACTIVE: %s %u.%u.%u\n", candidate->module_id, result.version_major, result.version_minor, result.version_patch);
    return 1;
}

void digit_hotload_init(digit_hotload_t *hotload, digit_module_manager_t *manager)
{
    if (hotload == NULL) return;
    memset(hotload, 0, sizeof(*hotload));
    hotload->manager = manager;
}

int digit_hotload_snapshot(digit_hotload_t *hotload)
{
    size_t count;
    if (hotload == NULL) return 0;
    if (!digit_hotload_collect(hotload, hotload->files, &count)) return 0;
    hotload->count = count;
    return 1;
}

int digit_hotload_poll(digit_hotload_t *hotload)
{
    digit_hotload_file_t current[DIGIT_HOTLOAD_MAX_MODULES];
    size_t current_count;
    size_t index;
    int changes = 0;
    if (hotload == NULL || hotload->manager == NULL) return -1;
    if (!digit_hotload_collect(hotload, current, &current_count)) return -1;
    for (index = 0; index < current_count; ++index)
    {
        const digit_hotload_file_t *previous = digit_hotload_find(hotload->files, hotload->count, current[index].module_id);
        if (previous == NULL)
        {
            printf("[MODULE] New candidate detected: %s\n", current[index].module_id);
            if (digit_hotload_promote(hotload, &current[index])) ++changes;
        }
        else if (previous->modified_time != current[index].modified_time || previous->size != current[index].size)
        {
            printf("[MODULE] Update candidate detected: %s\n", current[index].module_id);
            if (digit_hotload_promote(hotload, &current[index])) ++changes;
        }
    }
    memcpy(hotload->files, current, current_count * sizeof(current[0]));
    hotload->count = current_count;
    return changes;
}
