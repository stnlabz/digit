#include <stdio.h>
#include <string.h>
#include "interface_audit.h"

/* [AI:GPT-6 | 2026-10-08] 1.3.9: authorized and rejected
 * event coverage with absence of sensitive payload data. */
static unsigned int total,failed;
static void check(int ok,const char *label){
    ++total;
    printf("%s 1.3.9 %02u - %s\n",ok?"PASS":"FAIL",total,label);
    if(!ok)++failed;
}
static int event(const char *req,int status,const char *expected){
    char buf[160];
    return digit_interface_audit_format(req,status,buf,sizeof(buf)) &&
           strcmp(buf,expected)==0;
}
int main(void){
    char buf[160],small[8];
    check(event("GET /health HTTP/1.1\r\n\r\n",200,
          "[INTERFACE AUDIT] operation=health outcome=SUCCESS status=200"),
          "successful health request classified");
    check(event("POST /session/login HTTP/1.1\r\n\r\n",200,
          "[INTERFACE AUDIT] operation=session_login outcome=SUCCESS status=200"),
          "successful login classified");
    check(event("POST /session/login HTTP/1.1\r\n\r\n",401,
          "[INTERFACE AUDIT] operation=session_login outcome=REJECTED status=401"),
          "denied login classified");
    check(event("POST /session/logout HTTP/1.1\r\n\r\n",200,
          "[INTERFACE AUDIT] operation=session_logout outcome=SUCCESS status=200"),
          "logout classified");
    check(event("GET /session HTTP/1.1\r\n\r\n",401,
          "[INTERFACE AUDIT] operation=session_check outcome=REJECTED status=401"),
          "missing session classified");
    check(event("POST /projects/org/secret/bind HTTP/1.1\r\n\r\n",404,
          "[INTERFACE AUDIT] operation=project_binding outcome=REJECTED status=404"),
          "project binding denial classified");
    check(event("GET /channels/private-id HTTP/1.1\r\n\r\n",404,
          "[INTERFACE AUDIT] operation=channel_access outcome=REJECTED status=404"),
          "channel denial classified");
    check(event("GET /channels/private-id HTTP/1.1\r\n\r\n",200,
          "[INTERFACE AUDIT] operation=channel_access outcome=SUCCESS status=200"),
          "channel success classified");
    check(event("POST /channels HTTP/1.1\r\n\r\n",403,
          "[INTERFACE AUDIT] operation=channel_provision outcome=REJECTED status=403"),
          "provisioning prohibition classified");
    check(event("GET /alerts HTTP/1.1\r\n\r\n",503,
          "[INTERFACE AUDIT] operation=alert_access outcome=ERROR status=503"),
          "service failure classified");
    check(event("POST /ask HTTP/1.1\r\n\r\n",200,
          "[INTERFACE AUDIT] operation=request_dispatch outcome=SUCCESS status=200"),
          "dispatch classified");
    check(event("GET /corpus/sensitive-record HTTP/1.1\r\n\r\n",200,
          "[INTERFACE AUDIT] operation=corpus_access outcome=SUCCESS status=200"),
          "corpus read classified");
    check(event("POST /input HTTP/1.1\r\n\r\n",400,
          "[INTERFACE AUDIT] operation=corpus_input outcome=REJECTED status=400"),
          "input validation rejection classified");
    check(event(NULL,400,
          "[INTERFACE AUDIT] operation=invalid_request outcome=REJECTED status=400"),
          "malformed request classified");
    check(event("GET /bad?token=secret HTTP/1.1\r\n\r\n",404,
          "[INTERFACE AUDIT] operation=unknown_route outcome=REJECTED status=404"),
          "unknown URL details excluded");
    check(digit_interface_audit_format(
          "POST /channels/private-id HTTP/1.1\r\nAuthorization: Bearer confidential\r\n\r\nsecretbody",
          404,buf,sizeof(buf)) &&
          !strstr(buf,"confidential")&&!strstr(buf,"private-id")&&!strstr(buf,"secretbody"),
          "credential resource and body excluded");
    check(!digit_interface_audit_format("GET /health HTTP/1.1",200,small,sizeof(small)),
          "small audit buffer fails closed");
    check(!digit_interface_audit_format("GET /health HTTP/1.1",0,buf,sizeof(buf)),
          "invalid status rejected");
    check(!digit_interface_audit_format(NULL,200,NULL,0),
          "missing output rejected");
    printf("Interface 1.3.9 milestone: %u executed, %u failed\n",total,failed);
    return failed!=0;
}
