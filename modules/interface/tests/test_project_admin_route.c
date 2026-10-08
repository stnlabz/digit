#include <stdio.h>
#include <string.h>
#include "project_admin_route.h"

/* [AI:GPT-6 | 2026-10-08] Exact project administration route tests. */
static int total=0,failed=0;
static void check(int ok,const char *label) {
    ++total;
    printf("%s %02d - %s\n",ok?"PASS":"FAIL",total,label);
    if(!ok)++failed;
}
int main(void) {
    char org[DIGIT_PROJECT_ID_MAX],project[DIGIT_PROJECT_ID_MAX];
    const char *valid="POST /projects/stn-labz/digit/security/bind HTTP/1.1\r\nHost: localhost\r\n";
    check(digit_project_bind_route(valid,org,sizeof(org),project,sizeof(project)),"authorized method route parsed");
    check(strcmp(org,"stn-labz")==0,"organization preserved");
    check(strcmp(project,"digit")==0,"project preserved");
    check(digit_project_bind_route("POST /projects/stn-labz/digit/security/bind HTTP/1.1",org,sizeof(org),project,sizeof(project)),"bare request line parsed");
    check(!digit_project_bind_route("GET /projects/stn-labz/digit/security/bind HTTP/1.1",org,sizeof(org),project,sizeof(project)),"GET cannot bind");
    check(!digit_project_bind_route("POST /projects/stn-labz/digit/security/bind HTTP/1.0",org,sizeof(org),project,sizeof(project)),"wrong protocol denied");
    check(!digit_project_bind_route("POST /projects/stn-labz/digit/security/bind/extra HTTP/1.1",org,sizeof(org),project,sizeof(project)),"suffix traversal denied");
    check(!digit_project_bind_route("POST /projects/stn-labz/../security/bind HTTP/1.1",org,sizeof(org),project,sizeof(project)),"path traversal denied");
    check(!digit_project_bind_route("POST /projects/stn-labz/digit/security/bind HTTP/1.1extra",org,sizeof(org),project,sizeof(project)),"extra request text denied");
    check(!digit_project_bind_route("POST /projects/stn-labz//security/bind HTTP/1.1",org,sizeof(org),project,sizeof(project)),"empty project denied");
    check(!digit_project_bind_route("POST /projects/stn-labz/digit/security/bind%20HTTP/1.1",org,sizeof(org),project,sizeof(project)),"encoded bypass denied");
    check(!digit_project_bind_route(valid,org,2,project,sizeof(project)),"small organization buffer denied");
    check(!digit_project_bind_route(valid,org,sizeof(org),project,2),"small project buffer denied");
    check(!digit_project_bind_route(NULL,org,sizeof(org),project,sizeof(project)),"null request denied");
    check(!digit_project_bind_route(valid,NULL,sizeof(org),project,sizeof(project)),"null organization buffer denied");
    check(!digit_project_bind_route(valid,org,sizeof(org),NULL,sizeof(project)),"null project buffer denied");
    printf("Project admin route: %d executed, %d failed\n",total,failed);
    return failed!=0;
}
