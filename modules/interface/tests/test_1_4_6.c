#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http_write.h"
#include "knowledge_query.h"
/* [AI:GPT-6 | 2026-10-08] Interface 1.4.6:
 * exercise HTTP transport failure paths and legacy Corpus boundary contract. */
static unsigned int count,failed;
static void check(int good,const char *name)
{
    ++count;
    printf("%s 1.4.6 %02u - %s\n",good?"PASS":"FAIL",count,name);
    if(!good)++failed;
}
static digit_knowledge_record_t record(void)
{
    digit_knowledge_record_t r={0};
    strcpy(r.id,"record-1");
    strcpy(r.category,"engineering");
    strcpy(r.source,"manual:1");
    strcpy(r.text,"Evidence");
    return r;
}
int main(void)
{
    int pair[2];
    char buf[4096],json[10000];
    ssize_t n;
    digit_knowledge_record_t r=record(),records[2];
    check(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0,"socketpair created");
    check(digit_interface_http_write(pair[0],200,"{\"ok\":true}\n"),
          "complete HTTP response sent");
    n=recv(pair[1],buf,sizeof(buf)-1,0);
    check(n>0,"response received");
    if(n>0)buf[n]=0;else buf[0]=0;
    check(strstr(buf,"HTTP/1.1 200 OK\r\n")!=NULL,"HTTP status intact");
    check(strstr(buf,"Content-Length: 12\r\n")!=NULL,
          "response body length reported");
    check(strstr(buf,"\r\n\r\n{\"ok\":true}\n")!=NULL,
          "response header and JSON body intact");
    close(pair[1]);
    check(!digit_interface_http_write(pair[0],200,"after disconnect"),
          "disconnected peer returns failure without SIGPIPE");
    close(pair[0]);
    check(!digit_interface_http_write(-1,200,"x"),
          "invalid socket denied");
    check(!digit_interface_http_write(-1,200,NULL),
          "null response body denied");
    check(!digit_interface_http_send_all(-1,"data",4),
          "invalid write descriptor denied");
    check(!digit_interface_http_send_all(0,NULL,1),
          "null write buffer denied");
    check(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0,
          "second local response socket created");
    check(digit_interface_http_write(pair[0],503,"{}\n"),
          "service-unavailable response sent");
    n=recv(pair[1],buf,sizeof(buf)-1,0);
    if(n>0)buf[n]=0;else buf[0]=0;
    check(strstr(buf,"HTTP/1.1 503 Service Unavailable\r\n")!=NULL,
          "service failure status preserved");
    close(pair[0]);close(pair[1]);
    check(!digit_knowledge_query_valid("bad\nquery"),
          "legacy Corpus query control character denied");
    check(!digit_knowledge_record_id_valid("../x"),
          "legacy Corpus record path traversal denied");
    records[0]=r;records[1]=r;
    check(!digit_knowledge_results_valid(records,2),
          "duplicate Corpus evidence rejected");
    records[1]=r;strcpy(records[1].id,"record-2");
    check(digit_knowledge_result_json(records,2,json,sizeof(json)) &&
          strstr(json,"\"evidence_count\":2")!=NULL,
          "bounded source-attributed Corpus results accepted");
    records[1].source[0]=0;
    check(!digit_knowledge_result_json(records,2,json,sizeof(json)) &&
          json[0]==0,"unsourced Corpus result fails closed");
    check(!digit_knowledge_results_valid(&r,DIGIT_KNOWLEDGE_RECORD_MAX+1),
          "oversized Corpus result count denied before iteration");
    check(digit_knowledge_single_json(NULL,0,json,sizeof(json)) &&
          strstr(json,"\"answer\":\"UNKNOWN\"")!=NULL,
          "missing Corpus record explicit UNKNOWN");
    printf("Interface 1.4.6 milestone: %u executed, %u failed\n",count,failed);
    return failed!=0;
}
