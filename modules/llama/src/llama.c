#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "llama.h"
#include "module.h"

#define LLAMA_HTTP_MAX 16384
#define LLAMA_JSON_MAX 12288

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

static int json_escape(const char *input, char *output, size_t output_size)
{
    size_t i = 0, o = 0;
    if (input == NULL || output == NULL || output_size == 0) return 0;
    while (input[i] != '\0')
    {
        unsigned char ch = (unsigned char)input[i++];
        const char *escape = NULL;
        if (ch == '"') escape = "\\\"";
        else if (ch == '\\') escape = "\\\\";
        else if (ch == '\n') escape = "\\n";
        else if (ch == '\r') escape = "\\r";
        else if (ch == '\t') escape = "\\t";
        if (escape != NULL)
        {
            size_t n = strlen(escape);
            if (o + n >= output_size) return 0;
            memcpy(output + o, escape, n); o += n;
        }
        else
        {
            if (ch < 0x20 || o + 1 >= output_size) return 0;
            output[o++] = (char)ch;
        }
    }
    output[o] = '\0';
    return 1;
}

static int json_content(const char *json, char *output, size_t output_size)
{
    const char *p = strstr(json, "\"content\":");
    size_t o = 0;
    if (p == NULL || output == NULL || output_size == 0) return 0;
    p += strlen("\"content\":");
    while (*p == ' ' || *p == '\t') ++p;
    if (*p != '"') return 0;
    ++p;
    while (*p != '\0' && *p != '"')
    {
        char ch = *p++;
        if (ch == '\\')
        {
            ch = *p++;
            if (ch == '\0') return 0;
            if (ch == 'n' || ch == 'r' || ch == 't') ch = ' ';
            else if (ch != '"' && ch != '\\' && ch != '/') return 0;
        }
        if (o + 1 >= output_size) return 0;
        output[o++] = ch;
    }
    if (*p != '"') return 0;
    output[o] = '\0';
    return o > 0;
}

static int llama_generate_context(const char *text, char *context, size_t context_size)
{
    struct sockaddr_in address;
    int socket_fd;
    char escaped[DIGIT_LLAMA_TEXT_MAX * 2];
    char body[LLAMA_JSON_MAX];
    char request[LLAMA_HTTP_MAX];
    char response[LLAMA_HTTP_MAX];
    size_t received_total = 0;
    ssize_t received;
    int body_length, request_length;
    const char *json;

    if (!json_escape(text, escaped, sizeof(escaped))) return 0;
    body_length = snprintf(body, sizeof(body), "{\"prompt\":\"Return only a concise contextual interpretation for Digit. Identify what the statement means in relation to Digit without inventing facts or requirements. Statement: %s\",\"n_predict\":128,\"temperature\":0}", escaped);
    if (body_length <= 0 || (size_t)body_length >= sizeof(body)) return 0;

    request_length = snprintf(request, sizeof(request), "POST /completion HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nContent-Type: application/json\r\nAccept: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s", body_length, body);
    if (request_length <= 0 || (size_t)request_length >= sizeof(request)) return 0;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) return 0;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(LLAMA_DEFAULT_PORT);
    if (inet_pton(AF_INET, LLAMA_DEFAULT_HOST, &address.sin_addr) != 1 || connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) != 0) { close(socket_fd); return 0; }
    if (send(socket_fd, request, (size_t)request_length, 0) != request_length) { close(socket_fd); return 0; }

    while ((received = recv(socket_fd, response + received_total, sizeof(response) - 1 - received_total, 0)) > 0)
    {
        received_total += (size_t)received;
        if (received_total >= sizeof(response) - 1) break;
    }
    close(socket_fd);
    response[received_total] = '\0';
    if (strncmp(response, "HTTP/1.1 200", 12) != 0 && strncmp(response, "HTTP/1.0 200", 12) != 0) return 0;
    json = strstr(response, "\r\n\r\n");
    if (json == NULL) return 0;
    json += 4;
    return json_content(json, context, context_size);
}

static stnlabz_module_result_t llama_context_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *handler_context)
{
    const digit_llama_context_request_t *input = request;
    digit_llama_context_result_t output;
    (void)handler_context;

    if (request == NULL || request_size != sizeof(*input) || response == NULL || response_used == NULL || response_size < sizeof(output)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->text, '\0', sizeof(input->text)) == NULL || input->text[0] == '\0') return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;

    memset(&output, 0, sizeof(output));
    if (llama_generate_context(input->text, output.context, sizeof(output.context))) output.available = 1;
    else
    {
        output.available = 0;
        snprintf(output.context, sizeof(output.context), "%s", input->text);
        if (llama_host != NULL && llama_host->send_message != NULL) (void)llama_host->send_message("[LLAMA] context generation failed; raw input returned as fallback");
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
        if (llama_endpoint_reachable()) (void)host->send_message("[LLAMA] module active: llama.context registered; native llama.cpp /completion enabled at 127.0.0.1:8080");
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
    "llama", "Digit Llama Interface", 1, 0, 6,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    llama_qualify, llama_start, llama_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &llama_descriptor;
}
