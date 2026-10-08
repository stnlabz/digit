#include <string.h>
#include "http_auth.h"

/* [AI:GPT-6 | 2026-10-08] Exact-case header parser with bounded request.
 * All other casing of the Authorization header is rejected by duplicate
 * detection, preventing conflicting credentials in a case-insensitive HTTP
 * environment. This parser accepts HTTP/1.1 request lines only.
 */
static int ascii_equal_ci(const char *a, const char *b, size_t n)
{
    size_t i;
    for(i=0;i<n;++i) {
        unsigned char x=(unsigned char)a[i], y=(unsigned char)b[i];
        if (x>='A' && x<='Z') x=(unsigned char)(x+('a'-'A'));
        if (y>='A' && y<='Z') y=(unsigned char)(y+('a'-'A'));
        if (x!=y) return 0;
    }
    return 1;
}
int digit_http_bearer_token(const char *request, char token[DIGIT_SESSION_TOKEN_SIZE])
{
    const char *line,*end;
    int count=0;
    if (token==NULL) return 0;
    token[0]='\0';
    if (request==NULL) return 0;
    line=strstr(request,"\r\n");
    if (line==NULL || line==request) return 0;
    line+=2;
    for(;;) {
        const char *colon,*value;
        size_t n;
        end=strstr(line,"\r\n");
        if(end==NULL) return 0;
        if (end==line) break;
        if (*line==' ' || *line=='\t') return 0;
        colon=(const char *)memchr(line,':',(size_t)(end-line));
        if (colon==NULL) return 0;
        if ((size_t)(colon-line)==13U && ascii_equal_ci(line,"Authorization",13U)) {
            ++count;
            if (count>1) return 0;
            value=colon+1;
            while(value<end && (*value==' ' || *value=='\t')) ++value;
            if ((size_t)(end-value)!=7U+64U ||
                memcmp(value,"Bearer ",7U)!=0) return 0;
            value+=7;
            for (n=0;n<64U;++n) {
                char v=value[n];
                if (!((v>='0'&&v<='9') || (v>='a'&&v<='f')))
                    return 0;
            }
            memcpy(token,value,64U);
            token[64]='\0';
        }
        line=end+2;
    }
    if (count!=1) token[0]='\0';
    return count==1;
}
int digit_http_resolve_identity(const digit_session_store_t *sessions,
                                const char *request, time_t now,
                                char *identity, size_t identity_size)
{
    char token[DIGIT_SESSION_TOKEN_SIZE];
    int ok;
    if (identity!=NULL && identity_size>0) identity[0]='\0';
    if (!digit_http_bearer_token(request,token)) return 0;
    ok=digit_session_resolve(sessions,token,now,identity,identity_size);
    memset(token,0,sizeof(token));
    return ok;
}
