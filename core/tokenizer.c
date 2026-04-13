#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../digit.h"

/**
 * validate_integrity
 * Checks string for Law 1 violations (Uppercase characters).
 * Returns 1 if clean, 0 if violation found.
 */
int validate_integrity(const char *data) {
    if (data == NULL) return 1;

    for (int i = 0; data[i] != '\0'; i++) {
        if (isupper((unsigned char)data[i])) {
            return 0; // Violation: Uppercase detected
        }
    }
    return 1; // Clean
}

/* The Lexer: Scans a word and assigns a Sovereignty Type */
token_t get_next_token(const char **input) {
    token_t token;
    token.type = TOKEN_LITERAL;
    memset(token.text, 0, sizeof(token.text));

    // Skip whitespace
    while (**input && isspace(**input)) (*input)++;

    if (**input == '\0') {
        token.type = TOKEN_EOF;
        return token;
    }

    // Capture the word
    int i = 0;
    while (**input && !isspace(**input) && i < 255) {
        token.text[i] = **input;
        
        // Instant Integrity Check: Law 1
        if (isupper((unsigned char)**input)) {
            token.type = TOKEN_VIOLATION;
        }
        
        i++;
        (*input)++;
    }
    token.text[i] = '\0';

    // Entity Scrubbing: Check for the unrecognized
    if (strcasestr(token.text, "isrb")) {
        token.type = TOKEN_ENTITY;
    }

    return token;
}
