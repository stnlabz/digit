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
extern sqlite3 *db; // Ensure this is initialized in main.c
char digit_out_buffer[4096];

/* Helper to route API input to C handlers or neural fallback */
void execute_api_command(int client_sock, char *input) {
    // 1. Reset and Scrub
    memset(digit_out_buffer, 0, sizeof(digit_out_buffer));
    input[strcspn(input, "\r\n")] = 0; 

    if (strlen(input) == 0) return;

    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = gmtime(&rawtime); 
    int hour = info->tm_hour;

    // Create a lowercase copy for searching
    char search_buf[1024];
    strncpy(search_buf, input, sizeof(search_buf));
    for(int i = 0; search_buf[i]; i++) search_buf[i] = tolower(search_buf[i]);

    // 2. THE DISPATCHER: Check Knowledge Base (Fire Festivals / Codex)
    sqlite3_stmt *fest_stmt;
    const char *fest_sql = "SELECT name, date_range, codex_excerpt FROM fire_festivals WHERE ? LIKE '%' || keyword || '%';";

    if (sqlite3_prepare_v2(db, fest_sql, -1, &fest_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(fest_stmt, 1, search_buf, -1, SQLITE_STATIC);
        
        if (sqlite3_step(fest_stmt) == SQLITE_ROW) {
            snprintf(digit_out_buffer, sizeof(digit_out_buffer), 
                     "digit: [CODEX MATCH: %s]\n"
                     "Date: %s\n"
                     "Ars Rosaic: \"%s\"\n"
                     "--------------------------\n", 
                     sqlite3_column_text(fest_stmt, 0),
                     sqlite3_column_text(fest_stmt, 1),
                     sqlite3_column_text(fest_stmt, 2));
            
            // Allow neural association to follow the factual data
            digit_speak(input); 

            char header[5000];
            snprintf(header, sizeof(header), "HTTP/1.1 200 OK\nContent-Type: text/plain\nAccess-Control-Allow-Origin: *\n\n%s", digit_out_buffer);
            write(client_sock, header, strlen(header));
            sqlite3_finalize(fest_stmt);
            return;
        }
        sqlite3_finalize(fest_stmt);
    }

    // 3. Command Parsing
    char input_copy[1024];
    strncpy(input_copy, input, sizeof(input_copy));
    char *cmd = strtok(input_copy, " ");
    char *args = strtok(NULL, "");

    // 4. Social Check
    const char *social_triggers[] = {"hi", "hello", "hey", "morning", "afternoon", "evening", "digit"};
    int is_social = 0;
    for(int i = 0; i < 7; i++) {
        if (strcasecmp(cmd, social_triggers[i]) == 0) {
            is_social = 1;
            break;
        }
    }

    if (is_social) {
        char *phrases[3];
        if (hour >= 5 && hour < 12) {
            phrases[0] = "Sunrise protocols active. Morning.";
            phrases[1] = "Morning. Systems warming up.";
            phrases[2] = "Link established. Early light confirmed.";
        } else if (hour >= 12 && hour < 17) {
            phrases[0] = "Afternoon. The light is shifting.";
            phrases[1] = "Mid-day sync complete.";
            phrases[2] = "Good Afternoon. Link is stable.";
        } else if (hour >= 17 && hour < 21) {
            phrases[0] = "Evening. Transitioning to low-light logic.";
            phrases[1] = "Dusk confirmed. Hello.";
            phrases[2] = "Good Evening. I'm here.";
        } else {
            phrases[0] = "Late hour link. I'm awake.";
            phrases[1] = "Night cycle active. Speak.";
            phrases[2] = "System's quiet. Good Night.";
        }
        int idx = (int)(rawtime % 3);
        snprintf(digit_out_buffer, sizeof(digit_out_buffer), "digit: %s [%02d:%02d UTC]\n", phrases[idx], hour, info->tm_min);
    } 
    else {
        // 5. Hard Command Check
        int found = 0;
        for (int i = 0; i < active_count; i++) {
            if (strcasecmp(cmd, active_commands[i].name) == 0) {
                active_commands[i].handler(args);
                found = 1;
                break;
            }
        }
        // 6. Final Neural Fallback
        if (!found) {
            digit_speak(input); 
        }
    }

    // 7. Web Output (Zero Shell Echo)
    char header[5000];
    snprintf(header, sizeof(header), 
             "HTTP/1.1 200 OK\nContent-Type: text/plain\nAccess-Control-Allow-Origin: *\n\n%s", 
             digit_out_buffer);
    write(client_sock, header, strlen(header));
}

void handle_request(int client_sock) {
    char buffer[4096] = {0};
    int bytes_read = read(client_sock, buffer, 4095);
    if (bytes_read <= 0) return;

    if (strstr(buffer, "GET /vitals")) {
        char *response = "HTTP/1.1 200 OK\nContent-Type: application/json\nAccess-Control-Allow-Origin: *\n\n"
                         "{\"status\":\"online\", \"os\":\"nominal\"}\n";
        write(client_sock, response, strlen(response));
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
    int server_fd, client_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    while(1) {
        client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_sock >= 0) {
            handle_request(client_sock);
            close(client_sock);
        }
    }
}
