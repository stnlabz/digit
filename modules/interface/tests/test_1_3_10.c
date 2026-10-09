#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "http_limits.h"
#include "interface_audit.h"
#include "security_sa.h"
#include "project_provision.h"

/* [AI:GPT-6 | 2026-10-08] Interface 1.3.10: focused integration
 * stabilization for framing, audit redaction, SA lifecycle, and
 * ownership boundaries. Existing suites cover their full fixtures. */
static unsigned int total,failed;
static void check(int ok,const char *label)
{
    ++total;
    printf("%s 1.3.10 %02u - %s\n",ok?"PASS":"FAIL",total,label);
    if(!ok)++failed;
}
static int framing(const char *request,size_t capacity,size_t expected)
{
    const char *end=strstr(request,"\r\n\r\n");
    size_t body=(size_t)-1;
    return end && digit_http_limits_headers(request,(size_t)(end-request)+2,
                                            capacity,&body) && body==expected;
}
static int write_roster(const char *path,const char *contents)
{
    FILE *f=fopen(path,"w");
    int ok;
    if(!f)return 0;
    ok=fputs(contents,f)>=0;
    if(fclose(f)!=0)ok=0;
    return ok && chmod(path,0600)==0;
}
int main(void)
{
    char event[160],registry[]="/tmp/digit-stabilize-XXXXXX";
    const char *good="GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n";
    const char *sensitive="POST /session/login HTTP/1.1\r\nAuthorization: Bearer secret\r\nContent-Length: 5\r\n\r\n";
    size_t length=0;
    int fd;
    check(framing(good,65536,0),"valid health framing accepted");
    check(framing(sensitive,65536,5),"authenticated request framing accepted");
    check(!framing("POST /ask HTTP/1.1\r\nContent-Length: -1\r\n\r\n",65536,0),
          "invalid request length rejected");
    check(!framing("POST /ask HTTP/1.1\r\nContent-Length: 1\r\nContent-Length: 1\r\n\r\n",65536,1),
          "ambiguous framing rejected");
    check(!framing("POST /ask HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n",65536,0),
          "unimplemented transfer encoding rejected");
    check(!digit_http_limits_headers(NULL,10,65536,&length),
          "null HTTP input rejected");
    check(!digit_http_limits_headers(good,12,65536,NULL),
          "null HTTP output rejected");
    check(digit_interface_audit_format(sensitive,401,event,sizeof(event)) &&
          strstr(event,"operation=session_login") &&
          strstr(event,"outcome=REJECTED") &&
          strstr(event,"status=401") &&
          !strstr(event,"secret"),
          "denied login auditable without credentials");
    check(digit_interface_audit_format(good,200,event,sizeof(event)) &&
          strstr(event,"operation=health") && strstr(event,"outcome=SUCCESS"),
          "successful health request auditable");
    check(!digit_interface_audit_format(good,0,event,sizeof(event)),
          "invalid audit status rejected");
    fd=mkstemp(registry);
    if(fd<0)return 1;
    close(fd);
    check(write_roster(registry,"operator\torg\tSA\t1\t1\t1\n"),
          "private SA roster established");
    check(digit_security_sa_verify(registry,"org","operator"),
          "qualified current SA accepted");
    check(write_roster(registry,"operator\torg\tSA\t1\t0\t1\n"),
          "SA assignment revoked");
    check(!digit_security_sa_verify(registry,"org","operator"),
          "revoked SA rejected on next decision");
    check(write_roster(registry,"operator\torg\tSA\t1\t1\t1\n"),
          "SA assignment restored");
    check(digit_security_sa_verify(registry,"org","operator"),
          "restored SA accepted on next decision");
    check(digit_project_directory_owner_allowed(geteuid(),geteuid()),
          "runtime-owned project directory accepted");
    check(!digit_project_directory_owner_allowed((uid_t)-1,geteuid()),
          "invalid project owner denied");
    unlink(registry);
    printf("Interface 1.3.10 milestone: %u executed, %u failed\n",total,failed);
    return failed!=0;
}
