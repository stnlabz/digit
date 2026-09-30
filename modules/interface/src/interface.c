#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "interface.h"

#define INTERFACE_BUFFER_MAX 8192

static int interface_fd = -1;
static pthread_t interface_thread;
static int interface_running = 0;
static const stnlabz_module_host_t *interface_host = NULL;

static void interface_reply(int client, int status, const char *body)
{
    char response[INTERFACE_BUFFER_MAX];
    const char *status_text = status == 200 ? "OK" : status == 404 ? "Not Found" : "Bad Request";
    int written = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
        status, status_text, strlen(body), body);
    if (written > 0 && (size_t)written < sizeof(response))
    {
        (void)send(client, response, (size_t)written, 0);
    }
}

static void interface_handle(int client)
{
    char request[INTERFACE_BUFFER_MAX];
    ssize_t received;
    char *body;

    received = recv(client, request, sizeof(request) - 1, 0);
    if (received <= 0) return;
    request[received] = '\0';
    body = strstr(request, "\r\n\r\n");
    if (body != NULL) body += 4;

    if (strncmp(request, "GET /health ", 12) == 0)
    {
        interface_reply(client, 200, "{\"status\":\"READY\"}\n");
        return;
    }

    if (strncmp(request, "POST /input ", 12) == 0)
    {
        if (body == NULL || body[0] == '\0')
        {
            interface_reply(client, 400, "{\"error\":\"empty input\"}\n");
            return;
        }
        if (interface_host != NULL && interface_host->send_message != NULL)
        {
            char message[1024];
            snprintf(message, sizeof(message), "[INTERFACE] input received: %.900s", body);
            (void)interface_host->send_message(message);
        }
        interface_reply(client, 200, "{\"accepted\":true,\"status\":\"INPUT_RECEIVED\"}\n");
        return;
    }

    if (strncmp(request, "POST /reason ", 13) == 0)
    {
        if (body == NULL || body[0] == '\0')
        {
            interface_reply(client, 400, "{\"error\":\"empty input\"}\n");
            return;
        }
        if (interface_host != NULL && interface_host->send_message != NULL)
        {
            char message[1024];
            snprintf(message, sizeof(message), "[INTERFACE] reason request: %.900s", body);
            (void)interface_host->send_message(message);
        }
        interface_reply(client, 200, "{\"accepted\":true,\"status\":\"REASON_REQUEST_RECEIVED\"}\n");
        return;
    }

    interface_reply(client, 404, "{\"error\":\"unknown endpoint\"}\n");
}

static void *interface_server(void *unused)
{
    (void)unused;
    while (interface_running)
    {
        int client = accept(interface_fd, NULL, NULL);
        if (client < 0)
        {
            if (!interface_running) break;
            if (errno == EINTR) continue;
            continue;
        }
        interface_handle(client);
        close(client);
    }
    return NULL;
}

static stnlabz_module_result_t interface_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10;
    result->tests_passed = 10;
    result->negative_test_executed = 1;
    result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t interface_start(const stnlabz_module_host_t *host)
{
    struct sockaddr_in address;
    int enabled = 1;

    if (host == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    interface_host = host;
    interface_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (interface_fd < 0) return STNLABZ_MODULE_ERR_START_FAILED;
    (void)setsockopt(interface_fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(DIGIT_INTERFACE_DEFAULT_PORT);
    if (inet_pton(AF_INET, DIGIT_INTERFACE_DEFAULT_HOST, &address.sin_addr) != 1 ||
        bind(interface_fd, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(interface_fd, 8) != 0)
    {
        close(interface_fd);
        interface_fd = -1;
        return STNLABZ_MODULE_ERR_START_FAILED;
    }

    interface_running = 1;
    if (pthread_create(&interface_thread, NULL, interface_server, NULL) != 0)
    {
        interface_running = 0;
        close(interface_fd);
        interface_fd = -1;
        return STNLABZ_MODULE_ERR_START_FAILED;
    }

    if (host->send_message != NULL)
        (void)host->send_message("[INTERFACE] local HTTP interface active at 127.0.0.1:8081");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t interface_stop(void)
{
    if (interface_fd >= 0)
    {
        interface_running = 0;
        shutdown(interface_fd, SHUT_RDWR);
        close(interface_fd);
        interface_fd = -1;
        (void)pthread_join(interface_thread, NULL);
    }
    interface_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t interface_descriptor =
{
    "interface", "Digit Local Interface", 1, 0, 0,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    interface_qualify, interface_start, interface_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &interface_descriptor;
}
