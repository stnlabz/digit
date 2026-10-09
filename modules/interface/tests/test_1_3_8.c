#include <stdio.h>
#include <string.h>
#include "http_limits.h"

/* [AI:GPT-6 | 2026-10-08] 1.3.8 only: request framing limits. */
static unsigned int total,failed;
static void check(int good,const char *label){
    ++total;
    printf("%s 1.3.8 %02u - %s\n",good?"PASS":"FAIL",total,label);
    if(!good)++failed;
}
static int accepts(const char *request,size_t capacity,size_t want){
    const char *end=strstr(request,"\r\n\r\n");
    size_t actual=(size_t)-1;
    return end && digit_http_limits_headers(request,
                (size_t)(end-request)+2,capacity,&actual) && actual==want;
}
int main(void){
    char huge[9001],nul[96];
    const char *base="GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n";
    check(accepts(base,65536,0),"valid GET with headers");
    check(accepts("GET /health HTTP/1.1\r\n\r\n",65536,0),
          "valid GET without headers");
    check(accepts("POST /x HTTP/1.1\r\nContent-Length: 4\r\n\r\nbody",
                  65536,4),"valid bounded body length");
    check(!accepts("POST /x HTTP/1.1\r\nContent-Length: 4\r\nContent-Length: 4\r\n\r\nbody",65536,4),
          "duplicate Content-Length rejected");
    check(!accepts("POST /x HTTP/1.1\r\nContent-Length: -1\r\n\r\n",65536,0),
          "negative Content-Length rejected");
    check(!accepts("POST /x HTTP/1.1\r\nContent-Length: 1x\r\n\r\n",65536,0),
          "non-numeric Content-Length rejected");
    check(!accepts("POST /x HTTP/1.1\r\nContent-Length: 999999999999999999999\r\n\r\n",65536,0),
          "oversized Content-Length rejected");
    check(!accepts("POST /x HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n",65536,0),
          "unsupported chunked framing rejected");
    check(!accepts("GET /health HTTP/1.1\r\n Host: localhost\r\n\r\n",65536,0),
          "folded or whitespace-prefixed header rejected");
    check(!accepts("GET /health HTTP/1.0\r\nHost: localhost\r\n\r\n",65536,0),
          "unsupported HTTP version rejected");
    check(!accepts("GET /health HTTP/1.1\r\nHost localhost\r\n\r\n",65536,0),
          "malformed header denied");
    check(!accepts(base,strlen(base),0),
          "insufficient request buffer denied");
    memset(huge,'a',sizeof(huge));
    memcpy(huge,"GET /health HTTP/1.1\r\nX-Large: ",31);
    memcpy(huge+sizeof(huge)-5,"\r\n\r\n",4);
    huge[sizeof(huge)-1]=0;
    check(!accepts(huge,65536,0),"header over 8 KiB rejected");
    strcpy(nul,"GET /health HTTP/1.1\r\nHost: a\r\n\r\n");
    nul[24]=0;
    {
        size_t len=strlen("GET /health HTTP/1.1\r\nHost: a\r\n\r\n")-2;
        size_t body=0;
        check(!digit_http_limits_headers(nul,len,65536,&body),
              "embedded NUL header denied");
    }
    printf("Interface 1.3.8 milestone: %u executed, %u failed\n",total,failed);
    return failed!=0;
}
