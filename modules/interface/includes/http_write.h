#ifndef DIGIT_INTERFACE_HTTP_WRITE_H
#define DIGIT_INTERFACE_HTTP_WRITE_H
#include <stddef.h>
/* [AI:GPT-6 | 2026-10-08] Interface 1.4.6: bounded
 * HTTP replies; disconnected clients cannot raise SIGPIPE. */
int digit_interface_http_send_all(int fd,const char *data,size_t size);
int digit_interface_http_write(int fd,int status,const char *body);
#endif
