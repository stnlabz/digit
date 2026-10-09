#ifndef DIGIT_HTTP_LIMITS_H
#define DIGIT_HTTP_LIMITS_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-08] HTTP/1.1 framed request header validation.
 * header_length excludes the final CRLFCRLF; body bytes are bounded
 * by caller capacity including its required terminating NUL. */
#define DIGIT_HTTP_HEADER_MAX 8192u
int digit_http_limits_headers(const char *request,size_t header_length,
                             size_t capacity,size_t *body_length);
#endif
