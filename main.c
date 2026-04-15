#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include "digit.h"

/* Externs */
extern sqlite3 *db;
extern command_t active_commands[];
extern int active_count;
extern void update_weights(const char *content);
extern void save_weights_to_bin();
extern void digit_speak(const char *input);

/* Helper macro */
#define DIGIT_OUT(...) { \
    char temp_buf[4096]; \
    snprintf(temp_buf, sizeof(temp_buf), __VA_ARGS__); \
    strncat(digit_out_buffer, temp_buf, sizeof(digit_out_buffer) - strlen(digit_out_buffer) - 1); \
    printf("%s", temp_buf); \
}

/* --- HANDLER DEFINITIONS --- */

int cmd_help(const char *args) {
    DIGIT_OUT("\n--- digit protocol: active manifest ---\n");
    DIGIT_OUT("%-10s | %s\n", "COMMAND", "DESCRIPTION");
    DIGIT_OUT("-----------|--------------------------\n");

    if (logged_in && current_session_id > 0) {
        for (int i = 0; i < active_count; i++) {
            DIGIT_OUT("%-10s | %s\n", active_commands[i].name, active_commands[i].description);
        }
        DIGIT_OUT("-----------|--------------------------\n");
        DIGIT_OUT("status: %d utilities operational.\n\n", active_count);
    } else {
        DIGIT_OUT("%-10s | %s\n", "login", "authenticate");
        DIGIT_OUT("%-10s | %s\n", "help",  "view utilities");
        DIGIT_OUT("%-10s | %s\n", "chat",  "brain interface");
        DIGIT_OUT("-----------|--------------------------\n");
        DIGIT_OUT("status: 3 utilities operational (login required for more).\n\n");
    }
    return 0;
}

int cmd_login(const char *args) {
    if (!args || strlen(args) < 3) {
        DIGIT_OUT("digit: usage: login <username> <passphrase>\n");
        return 0;
    }

    char username[128], passphrase[128];
    if (sscanf(args, "%127s %127s", username, passphrase) != 2) {
        DIGIT_OUT("digit: invalid format. use: login <username> <passphrase>\n");
        return 0;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT 1 FROM users WHERE username = ? AND passphrase = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        DIGIT_OUT("digit: database error during login.\n");
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, passphrase, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        logged_in = 1;
        current_session_id = time(NULL);
        DIGIT_OUT("digit: authentication successful. session secured.\n");
        DIGIT_OUT("welcome back, operator. full command set unlocked.\n");
    } else {
        sqlite3_finalize(stmt);
        DIGIT_OUT("digit: invalid username or passphrase.\n");
    }
    return 0;
}

int cmd_ingest(const char *args) {
    if (!args || strlen(args) == 0) {
        DIGIT_OUT("digit: provide content or filename to ingest.\n");
        return 0;
    }

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "data/%s", args);

    FILE *fp = fopen(file_path, "r");
    if (fp) {
        char line[1024];
        int lines_count = 0;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = 0;
            if (strlen(line) < 2) continue;

            update_weights(line);
            lines_count++;
        }

        fclose(fp);
        DIGIT_OUT("digit: file '%s' indexed. %d lines processed.\n", args, lines_count);
    } else {
        update_weights(args);
        DIGIT_OUT("digit: data recorded.\n");
    }

    return 0;
}

int cmd_chat(const char *args) {
    if (!args || strlen(args) == 0) {
        DIGIT_OUT("digit: ...?\n");
        return 0;
    }
    digit_speak(args);
    return 0;
}

/* ✅ FIXED THINK — DB → WEIGHTS */
int cmd_think(const char *args) {
    DIGIT_OUT("digit: beginning neural synthesis...\n");

    sqlite3_stmt *stmt;
    const char *sql = "SELECT message FROM chat_logs WHERE is_learned = 1";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        DIGIT_OUT("digit: failed to access memory.\n");
        return 0;
    }

    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *text = (const char *)sqlite3_column_text(stmt, 0);
        if (text && strlen(text) > 0) {
            update_weights(text);
            count++;
        }
    }

    sqlite3_finalize(stmt);

    DIGIT_OUT("digit: synthesis complete. %d memory threads integrated.\n", count);
    return 0;
}

/* ✅ FIXED STUDY — DB + WEIGHTS */
int handle_study(const char *args) {
    if (!args || strlen(args) == 0) {
        DIGIT_OUT("digit: nothing to study.\n");
        return 0;
    }

    /* 1. Store in DB */
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO chat_logs (username, message, is_learned) VALUES (?, ?, 1)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, "operator", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, args, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    /* 2. Update weights immediately */
    update_weights(args);

    DIGIT_OUT("digit: information absorbed and archived.\n");
    return 0;
}

int handle_save(const char *args) {
    save_weights_to_bin();
    DIGIT_OUT("digit: transient memory committed to disk.\n");
    return 0;
}

int cmd_exit(const char *args) {
    DIGIT_OUT("digit: killing session. link terminated.\n");
    return 1;
}

/* --- COMMAND REGISTRY --- */
command_t restricted_commands[] = {
    {"login",  "authenticate", cmd_login},
    {"help",   "view utilities", cmd_help},
    {"ingest", "data entry", cmd_ingest},
    {"chat",   "brain interface", cmd_chat},
    {"think",  "train weights", cmd_think},
    {"study",  "insert knowledge", handle_study},
    {"save",   "commit to bin", handle_save},
    {"exit",   "kill session", cmd_exit}
};

int get_restricted_count() {
    return sizeof(restricted_commands) / sizeof(restricted_commands[0]);
}
