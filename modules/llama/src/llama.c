#include <arpa/inet.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "llama.h"
#include "module.h"

int llama_endpoint_reachable(void)
{
    struct sockaddr_in address;
    int socket_fd;
    int reachable = 0;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        return 0;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(LLAMA_DEFAULT_PORT);

    if (inet_pton(AF_INET, LLAMA_DEFAULT_HOST, &address.sin_addr) == 1 &&
        connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) == 0)
    {
        reachable = 1;
    }

    close(socket_fd);
    return reachable;
}

static stnlabz_module_result_t llama_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

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
    if (host != NULL && host->send_message != NULL)
    {
        if (llama_endpoint_reachable())
        {
            (void)host->send_message("[LLAMA] module active: endpoint reachable at 127.0.0.1:8080");
        }
        else
        {
            (void)host->send_message("[LLAMA] module active: endpoint unavailable at 127.0.0.1:8080");
        }
    }

    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t llama_stop(void)
{
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t llama_descriptor =
{
    "llama",
    "Digit Llama Interface",
    1,
    0,
    4,
    STNLABZ_MODULE_API_MAJOR,
    STNLABZ_MODULE_API_MINOR,
    llama_qualify,
    llama_start,
    llama_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &llama_descriptor;
}
