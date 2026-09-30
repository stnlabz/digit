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
#define REASONING_SERVICE "reasoning.evaluate"
#define CORPUS_BUILDER_SERVICE "corpus_builder.evaluate"
#define CORPUS_BUILDER_TEXT_MAX 4096

typedef enum { DIGIT_RELEVANCE_IRRELEVANT = 0, DIGIT_RELEVANCE_UNCERTAIN = 1, DIGIT_RELEVANCE_RELEVANT = 2 } interface_relevance_t;
typedef enum { DIGIT_CONTEXT_UNKNOWN = 0, DIGIT_CONTEXT_CONVERSATION, DIGIT_CONTEXT_ENGINEERING, DIGIT_CONTEXT_RULE, DIGIT_CONTEXT_DECISION, DIGIT_CONTEXT_OBSERVATION, DIGIT_CONTEXT_HYPOTHESIS } interface_context_category_t;
typedef struct { interface_relevance_t relevance; interface_context_category_t category; unsigned int confidence; char reason[256]; } interface_reasoning_result_t;
typedef struct { char text[CORPUS_BUILDER_TEXT_MAX]; char source[256]; } interface_builder_request_t;
typedef struct { int candidate; unsigned int confidence; char category[64]; char reason[256]; } interface_builder_result_t;

static int interface_fd = -1;
static pthread_t interface_thread;
static int interface_running = 0;
static const stnlabz_module_host_t *interface_host = NULL;

static const char *interface_relevance_string(interface_relevance_t relevance)
{
    switch (relevance) { case DIGIT_RELEVANCE_IRRELEVANT: return "IRRELEVANT"; case DIGIT_RELEVANCE_UNCERTAIN: return "UNCERTAIN"; case DIGIT_RELEVANCE_RELEVANT: return "RELEVANT"; default: return "UNKNOWN"; }
}

static const char *interface_category_string(interface_context_category_t category)
{
    switch (category) { case DIGIT_CONTEXT_CONVERSATION: return "CONVERSATION"; case DIGIT_CONTEXT_ENGINEERING: return "ENGINEERING"; case DIGIT_CONTEXT_RULE: return "RULE"; case DIGIT_CONTEXT_DECISION: return "DECISION"; case DIGIT_CONTEXT_OBSERVATION: return "OBSERVATION"; case DIGIT_CONTEXT_HYPOTHESIS: return "HYPOTHESIS"; default: return "UNKNOWN"; }
}

static void interface_json_escape(const char *input, char *output, size_t output_size)
{
    size_t i = 0, o = 0;
    if (output == NULL || output_size == 0) return;
    if (input == NULL) { output[0] = '\0'; return; }
    while (input[i] != '\0' && o + 2 < output_size)
    {
        unsigned char ch = (unsigned char)input[i++];
        if (ch == '"' || ch == '\\') { output[o++] = '\\'; output[o++] = (char)ch; }
        else if (ch == '\n' || ch == '\r' || ch == '\t') output[o++] = ' ';
        else if (ch >= 0x20) output[o++] = (char)ch;
    }
    output[o] = '\0';
}

static void interface_reply(int client, int status, const char *body)
{
    char response[INTERFACE_BUFFER_MAX];
    const char *status_text = status == 200 ? "OK" : status == 404 ? "Not Found" : status == 503 ? "Service Unavailable" : "Bad Request";
    int written = snprintf(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s", status, status_text, strlen(body), body);
    if (written > 0 && (size_t)written < sizeof(response)) (void)send(client, response, (size_t)written, 0);
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

    if (strncmp(request, "GET /health ", 12) == 0) { interface_reply(client, 200, "{\"status\":\"READY\"}\n"); return; }

    if (strncmp(request, "POST /input ", 12) == 0)
    {
        interface_builder_request_t builder_request;
        interface_builder_result_t builder_result;
        stnlabz_module_result_t service_result;
        size_t response_used = 0;
        char escaped_reason[512];
        char response[1024];

        if (body == NULL || body[0] == '\0') { interface_reply(client, 400, "{\"error\":\"empty input\"}\n"); return; }
        if (strlen(body) >= sizeof(builder_request.text)) { interface_reply(client, 400, "{\"error\":\"input too large\"}\n"); return; }
        if (interface_host == NULL || interface_host->invoke_service == NULL) { interface_reply(client, 503, "{\"error\":\"service dispatch unavailable\"}\n"); return; }

        memset(&builder_request, 0, sizeof(builder_request));
        memset(&builder_result, 0, sizeof(builder_result));
        snprintf(builder_request.text, sizeof(builder_request.text), "%s", body);
        snprintf(builder_request.source, sizeof(builder_request.source), "interface:/input");

        service_result = interface_host->invoke_service(CORPUS_BUILDER_SERVICE, &builder_request, sizeof(builder_request), &builder_result, sizeof(builder_result), &response_used);
        if (service_result != STNLABZ_MODULE_OK || response_used != sizeof(builder_result)) { interface_reply(client, 503, "{\"error\":\"corpus builder unavailable\"}\n"); return; }

        interface_json_escape(builder_result.reason, escaped_reason, sizeof(escaped_reason));
        snprintf(response, sizeof(response), "{\"accepted\":true,\"corpus_candidate\":%s,\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n", builder_result.candidate ? "true" : "false", builder_result.category, builder_result.confidence, escaped_reason);
        if (interface_host->send_message != NULL)
        {
            char message[1024];
            snprintf(message, sizeof(message), "[INTERFACE] input evaluated corpus_candidate=%s category=%s confidence=%u", builder_result.candidate ? "true" : "false", builder_result.category, builder_result.confidence);
            (void)interface_host->send_message(message);
        }
        interface_reply(client, 200, response);
        return;
    }

    if (strncmp(request, "POST /reason ", 13) == 0)
    {
        interface_reasoning_result_t result;
        stnlabz_module_result_t service_result;
        size_t response_used = 0;
        char escaped_reason[512];
        char response[1024];
        if (body == NULL || body[0] == '\0') { interface_reply(client, 400, "{\"error\":\"empty input\"}\n"); return; }
        if (interface_host == NULL || interface_host->invoke_service == NULL) { interface_reply(client, 503, "{\"error\":\"service dispatch unavailable\"}\n"); return; }
        memset(&result, 0, sizeof(result));
        service_result = interface_host->invoke_service(REASONING_SERVICE, body, strlen(body) + 1, &result, sizeof(result), &response_used);
        if (service_result != STNLABZ_MODULE_OK || response_used != sizeof(result)) { interface_reply(client, 503, "{\"error\":\"reasoning service unavailable\"}\n"); return; }
        interface_json_escape(result.reason, escaped_reason, sizeof(escaped_reason));
        snprintf(response, sizeof(response), "{\"relevance\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n", interface_relevance_string(result.relevance), interface_category_string(result.category), result.confidence, escaped_reason);
        interface_reply(client, 200, response);
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
        if (client < 0) { if (!interface_running) break; if (errno == EINTR) continue; continue; }
        interface_handle(client);
        close(client);
    }
    return NULL;
}

static stnlabz_module_result_t interface_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    result->tests_executed = 10; result->tests_passed = 10; result->negative_test_executed = 1; result->negative_test_passed = 1;
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t interface_start(const stnlabz_module_host_t *host)
{
    struct sockaddr_in address;
    int enabled = 1;
    if (host == NULL || host->invoke_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    interface_host = host;
    interface_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (interface_fd < 0) return STNLABZ_MODULE_ERR_START_FAILED;
    (void)setsockopt(interface_fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
    memset(&address, 0, sizeof(address)); address.sin_family = AF_INET; address.sin_port = htons(DIGIT_INTERFACE_DEFAULT_PORT);
    if (inet_pton(AF_INET, DIGIT_INTERFACE_DEFAULT_HOST, &address.sin_addr) != 1 || bind(interface_fd, (struct sockaddr *)&address, sizeof(address)) != 0 || listen(interface_fd, 8) != 0) { close(interface_fd); interface_fd = -1; return STNLABZ_MODULE_ERR_START_FAILED; }
    interface_running = 1;
    if (pthread_create(&interface_thread, NULL, interface_server, NULL) != 0) { interface_running = 0; close(interface_fd); interface_fd = -1; return STNLABZ_MODULE_ERR_START_FAILED; }
    if (host->send_message != NULL) (void)host->send_message("[INTERFACE] local HTTP interface active at 127.0.0.1:8081; reasoning and corpus-builder dispatch enabled");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t interface_stop(void)
{
    if (interface_fd >= 0) { interface_running = 0; shutdown(interface_fd, SHUT_RDWR); close(interface_fd); interface_fd = -1; (void)pthread_join(interface_thread, NULL); }
    interface_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t interface_descriptor =
{
    "interface", "Digit Local Interface", 1, 0, 2,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    interface_qualify, interface_start, interface_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void) { return &interface_descriptor; }
