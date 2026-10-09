#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "http_write.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.6: send exact byte counts,
 * retry interruption, abort on closed peers and never signal SIGPIPE. */
int digit_interface_http_send_all(int fd,const char *data,size_t size)
{
    size_t sent=0;
    if(fd<0 || (!data && size))return 0;
    while(sent<size){
        ssize_t n=send(fd,data+sent,size-sent,MSG_NOSIGNAL);
        if(n>0){sent+=(size_t)n;continue;}
        if(n<0 && errno==EINTR)continue;
        return 0;
    }
    return 1;
}
int digit_interface_http_write(int fd,int status,const char *body)
{
    char header[512];
    const char *reason;
    int n;
    if(fd<0 || !body)return 0;
    reason=status==200?"OK":
           status==400?"Bad Request":
           status==401?"Unauthorized":
           status==403?"Forbidden":
           status==404?"Not Found":
           status==503?"Service Unavailable":"Bad Request";
    n=snprintf(header,sizeof(header),
       "HTTP/1.1 %d %s\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
       status,reason,strlen(body));
    if(n<=0 || (size_t)n>=sizeof(header))return 0;
    if(!digit_interface_http_send_all(fd,header,(size_t)n))return 0;
    return digit_interface_http_send_all(fd,body,strlen(body));
}
