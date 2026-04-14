#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "digit.h"

extern command_t active_commands[20];
extern int active_count;
extern sqlite3 *db;
extern int logged_in;
extern long current_session_id;

char digit_out_buffer[4096];

/* Simple session check */
int is_logged_in(void) {
    return (logged_in && current_session_id > 0);
}

/* ====================== MAIN COMMAND EXECUTOR ====================== */
void execute_api_command(int client_sock, char *input) {
    memset(digit_out_buffer, 0, sizeof(digit_out_buffer));
    input[strcspn(input, "\r\n")] = 0;

    if (strlen(input) == 0) return;

    char search_buf[1024];
    strncpy(search_buf, input, sizeof(search_buf));
    for (int i = 0; search_buf[i]; i++) search_buf[i] = tolower(search_buf[i]);

    /* Festival check - always allowed */
    sqlite3_stmt *fest_stmt;
    const char *fest_sql = "SELECT name, date_range, codex_excerpt FROM fire_festivals WHERE keyword LIKE '%' || ? || '%';";

    if (sqlite3_prepare_v2(db, fest_sql, -1, &fest_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(fest_stmt, 1, search_buf, -1, SQLITE_STATIC);
        if (sqlite3_step(fest_stmt) == SQLITE_ROW) {
            snprintf(digit_out_buffer, sizeof(digit_out_buffer),
                     "digit: [CODEX MATCH: %s]\nDate: %s\nArs Rosaic: \"%s\"\n",
                     sqlite3_column_text(fest_stmt, 0),
                     sqlite3_column_text(fest_stmt, 1),
                     sqlite3_column_text(fest_stmt, 2));
            digit_speak(input);
            goto send_response;
        }
        sqlite3_finalize(fest_stmt);
    }

    /* Parse command */
    char input_copy[1024];
    strncpy(input_copy, input, sizeof(input_copy));
    char *cmd = strtok(input_copy, " ");
    char *args = strtok(NULL, "");

    /* ==================== LOGIN GATE ==================== */
    if (!is_logged_in()) {
        if (strcasecmp(cmd, "login") != 0 &&
            strcasecmp(cmd, "help") != 0 &&
            strcasecmp(cmd, "chat") != 0) {

            snprintf(digit_out_buffer, sizeof(digit_out_buffer),
                     "digit: access restricted.\nUse 'login <username> <passphrase>' to authenticate.\n");
            goto send_response;
        }
    }
    /* ==================================================== */

    /* Social greetings */
    const char *social[] = {"hi", "hello", "hey", "morning", "afternoon", "evening", "digit"};
    int is_social = 0;
    for (int i = 0; i < 7; i++) {
        if (strcasecmp(cmd, social[i]) == 0) {
            is_social = 1;
            break;
        }
    }

    if (is_social) {
        time_t raw = time(NULL);
        struct tm *t = gmtime(&raw);
        snprintf(digit_out_buffer, sizeof(digit_out_buffer),
                 "digit: Good %s. Link stable.\n", 
                 (t->tm_hour < 12) ? "morning" : (t->tm_hour < 17) ? "afternoon" : "evening");
    } 
    else {
        /* Run the actual command */
        int found = 0;
        for (int i = 0; i < active_count; i++) {
            if (strcasecmp(cmd, active_commands[i].name) == 0) {
                active_commands[i].handler(args);
                found = 1;
                break;
            }
        }
        if (!found) {
            digit_speak(input);
        }
    }

send_response:
    char header[5000];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\nContent-Type: text/plain\nAccess-Control-Allow-Origin: *\n\n%s",
             digit_out_buffer);
    write(client_sock, header, strlen(header));
}

/* ====================== SERVER ====================== */
void handle_request(int client_sock) {
    char buffer[4096] = {0};
    int bytes = read(client_sock, buffer, 4095);
    if (bytes <= 0) return;

    if (strstr(buffer, "GET /vitals")) {
        const char *resp = "HTTP/1.1 200 OK\nContent-Type: application/json\nAccess-Control-Allow-Origin: *\n\n"
                           "{\"status\":\"online\"}\n";
        write(client_sock, resp, strlen(resp));
    } 
    else if (strstr(buffer, "POST /chat")) {
        char *payload = strstr(buffer, "\r\n\r\n");
        if (payload) {
            payload += 4;
            execute_api_command(client_sock, payload);
        }
    }
}

void start_api_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return; }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        return;
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        return;
    }

    printf("digit api listening on port %d\n", port);

    while (1) {
        int client = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client >= 0) {
            handle_request(client);
            close(client);
        }
    }
}
