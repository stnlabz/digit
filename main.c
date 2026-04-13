#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "digit.h"

/* Global Database Handle */
sqlite3 *db = NULL;

/* Storage bumped to 20 to accommodate the 12+ commands */
command_t active_commands[20];
int active_count = 0;

/* External function from api.c */
extern void start_api_server(int port);

void load_core_commands() {
    active_count = 0;
    int r_count = get_restricted_count();

    for (int i = 0; i < r_count; i++) {
        active_commands[active_count++] = restricted_commands[i];
    }
}

int main(int argc, char *argv[]) {
    int port = 9000;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    if (sqlite3_open("data/digit.db", &db) != SQLITE_OK) {
        fprintf(stderr, "system: cannot open sovereign database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    load_weights_from_bin();
    load_core_commands();

    printf("digit online. port: %d\n", port);

    /* Start the API server - this blocks and handles the loop */
    start_api_server(port);

    sqlite3_close(db);
    return 0;
}
