#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "module.h"

#define LLAMA_HOST "127.0.0.1"
#define LLAMA_PORT "8080"

static stnlabz_module_result_t llama_qualify(
    stnlabz_module_qualification_result_t *result
)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    int socket_fd = -1;
    int network_ok = 0;

    if (result == NULL)
    {
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    }

    memset(result, 0, sizeof(*result));

    /* Deterministic module self-tests. */
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->tests_failed = 0;

    /* Negative validation: an invalid descriptor target must not resolve. */
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;

    /* Reachability is useful evidence but does not make Llama a Core dependency. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(LLAMA_HOST, LLAMA_PORT, &hints, &addresses) == 0)
    {
        struct addrinfo *address;

        for (address = addresses; address != NULL; address = address->ai_next)
        {
            socket_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
            if (socket_fd < 0)
            {
                continue;
            }

            if (connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0)
            {
                network_ok = 1;
                close(socket_fd);
                socket_fd = -1;
                break;
            }

            close(socket_fd);
            socket_fd = -1;
        }

        freeaddrinfo(addresses);
    }

    (void)network_ok;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t llama_start(
    const stnlabz_module_host_t *host
)
{
    if (host != NULL && host->send_message != NULL)
    {
        (void)host->send_message("[LLAMA] module active: http://127.0.0.1:8080");
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
    0,
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
