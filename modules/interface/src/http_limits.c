#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include "http_limits.h"

/* [AI:GPT-6 | 2026-10-08] Interface 1.3.8: reject ambiguous HTTP
 * framing, unsafe control characters and oversized header sections.
 * Transfer-Encoding is unsupported; no chunked interpretation. */
int digit_http_limits_headers(const char *request,size_t header_length,
                             size_t capacity,size_t *body_length)
{
    size_t i,pos,seen=0,length=0,first_line=0;
    if(!request||!body_length||header_length<14||
       header_length>DIGIT_HTTP_HEADER_MAX||header_length+4>=capacity)return 0;
    *body_length=0;
    for(i=0;i<header_length;++i){
        unsigned char c=(unsigned char)request[i];
        if(c==0 || c==127 || (c<32 && c!='\r' && c!='\n' && c!='\t'))return 0;
        if(c=='\r' && (i+1>=header_length||request[i+1]!='\n'))return 0;
        if(c=='\n' && (i==0||request[i-1]!='\r'))return 0;
    }
    for(i=0;i+1<header_length;++i){
        if(request[i]=='\r' && request[i+1]=='\n'){first_line=i;break;}
    }
    if(first_line<12||first_line>=header_length)return 0;
    if(!(first_line>=9 && memcmp(request+first_line-9," HTTP/1.1",9)==0))return 0;
    pos=first_line+2;
    while(pos<header_length){
        size_t end=pos,colon=pos,value;
        while(end+1<header_length &&
              !(request[end]=='\r'&&request[end+1]=='\n'))++end;
        if(end+1>=header_length || end==pos)return 0;
        while(colon<end && request[colon]!=':')++colon;
        if(colon==pos || colon==end)return 0;
        for(i=pos;i<colon;++i){
            unsigned char ch=(unsigned char)request[i];
            if(!((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||
                 (ch>='0'&&ch<='9')||ch=='-'))return 0;
        }
        value=colon+1;
        while(value<end && (request[value]==' '||request[value]=='\t'))++value;
        if(colon-pos==14 && strncasecmp(request+pos,"Content-Length",14)==0){
            size_t n=0;
            if(++seen!=1 || value==end)return 0;
            for(i=value;i<end;++i){
                unsigned char ch=(unsigned char)request[i];
                if(ch<'0'||ch>'9'||n>(capacity-(header_length+4)-1)/10)return 0;
                n=n*10+(size_t)(ch-'0');
                if(n>=capacity-(header_length+4))return 0;
            }
            length=n;
        }
        if(colon-pos==17 && strncasecmp(request+pos,"Transfer-Encoding",17)==0)
            return 0;
        pos=end+2;
    }
    if(pos!=header_length)return 0;
    *body_length=length;
    return 1;
}
