#include <stdio.h>
#include <string.h>
#include "http_auth.h"

/* [AI:GPT-6 | 2026-10-08] Strict header/session negative test suite. */
static unsigned int count=0, failures=0;
static void check(int condition,const char *label)
{
    ++count;
    if (condition) printf("PASS %02u - %s\n",count,label);
    else {++failures;printf("FAIL %02u - %s\n",count,label);}
}
int main(void)
{
    digit_session_store_t sessions;
    char token[DIGIT_SESSION_TOKEN_SIZE],parsed[DIGIT_SESSION_TOKEN_SIZE];
    char request[512],identity[DIGIT_SESSION_ID_SIZE],other[512];
    time_t now=1000;
    digit_session_store_init(&sessions);
    check(digit_session_issue(&sessions,"ezra",1,now,token),"verified session established");
    snprintf(request,sizeof(request),"GET /session HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer %s\r\n\r\n",token);
    check(digit_http_bearer_token(request,parsed) && strcmp(token,parsed)==0,
          "exact bearer extracted");
    check(digit_http_resolve_identity(&sessions,request,now,identity,sizeof(identity)) &&
          strcmp(identity,"ezra")==0,"session binds request to verified identity");
    check(!digit_http_bearer_token("GET /session HTTP/1.1\r\nHost: localhost\r\n\r\n",parsed),
          "missing Authorization denied");
    check(!digit_http_bearer_token("GET /session HTTP/1.1\r\nAuthorization: Bearer short\r\n\r\n",parsed),
          "short token denied");
    snprintf(other,sizeof(other),"GET /session HTTP/1.1\r\nAuthorization: Basic %s\r\n\r\n",token);
    check(!digit_http_bearer_token(other,parsed),"basic authentication denied");
    snprintf(other,sizeof(other),"GET /session HTTP/1.1\r\nauthorization: Bearer %s\r\n\r\n",token);
    check(digit_http_bearer_token(other,parsed),"case-insensitive header name accepted");
    snprintf(other,sizeof(other),"GET /session HTTP/1.1\r\nAuthorization: Bearer %s\r\naUtHoRiZaTiOn: Bearer %s\r\n\r\n",token,token);
    check(!digit_http_bearer_token(other,parsed),"duplicate Authorization denied regardless of casing");
    snprintf(other,sizeof(other),"GET /session HTTP/1.1\r\n Authorization: Bearer %s\r\n\r\n",token);
    check(!digit_http_bearer_token(other,parsed),"folded header denied");
    snprintf(other,sizeof(other),"GET /session HTTP/1.1\r\nAuthorization: Bearer %s \r\n\r\n",token);
    check(!digit_http_bearer_token(other,parsed),"trailing whitespace denied");
    check(!digit_http_bearer_token("GET /session HTTP/1.1\r\nAuthorization: Bearer bad\r\n",parsed),
          "missing header terminator denied");
    check(!digit_http_bearer_token(NULL,parsed),"null request denied");
    check(!digit_http_bearer_token(request,NULL),"null token buffer denied");
    check(!digit_http_resolve_identity(&sessions,request,now+DIGIT_SESSION_LIFETIME,identity,sizeof(identity)),
          "expired bearer session denied");
    check(digit_session_revoke(&sessions,token),"token can be revoked");
    check(!digit_http_resolve_identity(&sessions,request,now,identity,sizeof(identity)),
          "revoked bearer denied");
    check(!digit_http_resolve_identity(NULL,request,now,identity,sizeof(identity)),
          "missing server session store denied");
    check(!digit_http_resolve_identity(&sessions,request,now,NULL,0),
          "missing identity buffer denied");
    printf("\nHTTP auth tests: %u executed, %u failed\n",count,failures);
    return failures?1:0;
}
