#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "../digit.h"

/* Externs from main.c and brain.c */
extern sqlite3 *db;
extern command_t active_commands[];
extern int active_count;
extern void update_weights(const char *content);
extern void save_weights_to_bin();
extern void digit_speak(const char *input);
extern void train_from_source(sqlite3 *db);

/* Helper macro to handle dual output (Web Buffer + Local CLI) */
#define DIGIT_OUT(...) { \
    char temp_buf[4096]; \
    snprintf(temp_buf, sizeof(temp_buf), __VA_ARGS__); \
    strncat(digit_out_buffer, temp_buf, sizeof(digit_out_buffer) - strlen(digit_out_buffer) - 1); \
    printf("%s", temp_buf); \
}

/* --- HANDLER DEFINITIONS --- */

int cmd_login(const char *args) {
    DIGIT_OUT("digit: authentication protocol active. session secured.\n");
    return 0;
}

int cmd_help(const char *args) {
    DIGIT_OUT("\n--- digit protocol: active manifest ---\n");
    DIGIT_OUT("%-10s | %s\n", "COMMAND", "DESCRIPTION");
    DIGIT_OUT("-----------|--------------------------\n");

    for (int i = 0; i < active_count; i++) {
        DIGIT_OUT("%-10s | %s\n", 
                   active_commands[i].name, 
                   active_commands[i].description);
    }
    
    DIGIT_OUT("-----------|--------------------------\n");
    DIGIT_OUT("status: %d utilities operational.\n\n", active_count);
    return 0;
}

int cmd_set(const char *args) {
    if (args == NULL || strlen(args) == 0) {
        DIGIT_OUT("digit: set requires a rule string.\n");
        return 0;
    }

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO rules (rule_text) VALUES (?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        DIGIT_OUT("digit: failed to prepare rule update: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, args, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        DIGIT_OUT("digit: law established.\n");
    } else {
        DIGIT_OUT("digit: failed to commit law: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return 0;
}

int cmd_get(const char *args) {
    DIGIT_OUT("digit: rule retrieval is handled via the 'rules' command.\n");
    return 0;
}

int cmd_del(const char *args) {
    if (args == NULL) {
        DIGIT_OUT("digit: provide a rule ID to delete.\n");
        return 0;
    }

    char sql[256];
    sprintf(sql, "DELETE FROM rules WHERE id = %d;", atoi(args));
    
    char *err_msg = 0;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        DIGIT_OUT("digit: deletion failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        DIGIT_OUT("digit: law purged from record.\n");
    }
    return 0;
}

int cmd_rules(const char *args) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, rule_text FROM rules ORDER BY id ASC;";

    DIGIT_OUT("\n--- digit protocol: laws of the house ---\n");
    DIGIT_OUT("%-4s | %s\n", "ID", "LAW");
    DIGIT_OUT("-----|----------------------------------\n");

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        DIGIT_OUT("digit: error accessing laws: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DIGIT_OUT("%-4d | %s\n", sqlite3_column_int(stmt, 0), sqlite3_column_text(stmt, 1));
        found = 1;
    }

    sqlite3_finalize(stmt);
    if (!found) DIGIT_OUT("digit: no laws currently defined.\n");
    DIGIT_OUT("----------------------------------------\n\n");
    return 0;
}

int cmd_ingest(const char *args) {
    if (args == NULL || strlen(args) == 0) {
        DIGIT_OUT("digit: provide content or filename to ingest.\n");
        return 0;
    }

    // Check if the argument is a file in the data/ directory
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "data/%s", args);

    FILE *fp = fopen(file_path, "r");
    if (fp) {
        // --- FILE PROCESSING MODE ---
        char line[1024];
        int lines_count = 0;
        sqlite3_stmt *stmt;
        const char *sql = "INSERT INTO ingested_data (content) VALUES (?);";

        while (fgets(line, sizeof(line), fp)) {
            // Scrub newline for the DB
            line[strcspn(line, "\r\n")] = 0;
            if (strlen(line) < 2) continue;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, line, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                
                // Also update live memory weights so she "learns" immediately
                update_weights(line);
                lines_count++;
            }
        }
        fclose(fp);
        DIGIT_OUT("digit: file '%s' indexed. %d lines recorded to sovereign database.\n", args, lines_count);
    } 
    else {
        // --- DIRECT STRING MODE ---
        sqlite3_stmt *stmt;
        const char *sql = "INSERT INTO ingested_data (content) VALUES (?);";
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, args, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            // Update live weights
            update_weights(args);
            DIGIT_OUT("digit: data recorded to sovereign database.\n");
        }
    }
    return 0;
}

int cmd_chat(const char *args) {
    if (args == NULL) {
        DIGIT_OUT("digit: ...?\n");
        return 0;
    }
    /* Note: Ensure digit_speak is updated to write to digit_out_buffer internally */
    digit_speak(args);
    return 0;
}

int cmd_think(const char *args) {
    DIGIT_OUT("digit: beginning neural synthesis...\n");
    train_from_source(db);
    return 0;
}

int handle_study(const char *args) {
    if (args == NULL || strlen(args) == 0) {
        DIGIT_OUT("digit: nothing to study.\n");
        return 0;
    }

    update_weights(args);

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO ingested_data (content) VALUES (?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, args, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        DIGIT_OUT("digit: information absorbed and archived.\n");
    } else {
        DIGIT_OUT("digit: absorbed to RAM only.\n");
    }

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
    {"login",  "authenticate",      cmd_login},
    {"help",   "view utilities",    cmd_help},
    {"set",    "establish law",     cmd_set},
    {"get",    "view rule status",  cmd_get},
    {"del",    "purge law",         cmd_del},
    {"rules",  "list laws",         cmd_rules},
    {"ingest", "data entry",        cmd_ingest},
    {"chat",   "brain interface",   cmd_chat},
    {"think",  "train weights",     cmd_think},
    {"study",  "insert knowledge",  handle_study},
    {"save",   "commit to bin",     handle_save},
    {"exit",   "kill session",      cmd_exit}
};

int get_restricted_count() {
    return sizeof(restricted_commands) / sizeof(command_t);
}
