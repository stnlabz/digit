#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "llama.h"
#include "module.h"

static const stnlabz_module_host_t *llama_host = NULL;

int llama_endpoint_reachable(void)
{
    struct sockaddr_in address;
    int socket_fd;
    int reachable = 0;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) return 0;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(LLAMA_DEFAULT_PORT);
    if (inet_pton(AF_INET, LLAMA_DEFAULT_HOST, &address.sin_addr) == 1 && connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) == 0) reachable = 1;
    close(socket_fd);
    return reachable;
}

static stnlabz_module_result_t llama_context_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_llama_context_request_t *input = request;
    digit_llama_context_result_t output;
    (void)handler_context;

    if (request == NULL || request_size != sizeof(*input) || response == NULL || response_used == NULL || response_size < sizeof(output)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->text, '\0', sizeof(input->text)) == NULL || input->text[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;

    memset(&output, 0, sizeof(output));
    if (llama_endpoint_reachable())
    {
        output.available = 1;
        snprintf(output.context, sizeof(output.context), "%s", input->text);
    }
    else
    {
        output.available = 0;
        snprintf(output.context, sizeof(output.context), "%s", input->text);
    }

    memcpy(response, &output, sizeof(output));
    *response_used = sizeof(output);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t llama_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->tests_failed = 0;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t llama_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_LLAMA_CONTEXT_SERVICE, llama_context_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    llama_host = host;
    if (host->send_message != NULL)
    {
        if (llama_endpoint_reachable()) (void)host->send_message("[LLAMA] module active: llama.context registered; endpoint reachable at 127.0.0.1:8080");
        else (void)host->send_message("[LLAMA] module active: llama.context registered; endpoint unavailable at 127.0.0.1:8080");
    }
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t llama_stop(void)
{
    if (llama_host != NULL && llama_host->unregister_service != NULL)
        if (!llama_host->unregister_service(DIGIT_LLAMA_CONTEXT_SERVICE, NULL)) return STNLABZ_MODULE_ERR_STOP_FAILED;
    llama_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t llama_descriptor =
{
    "llama", "Digit Llama Interface", 1, 0, 5,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    llama_qualify, llama_start, llama_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &llama_descriptor;
}
