#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include "../src/stn2.c"
int main(void){
 stnlabz_module_qualification_result_t q={0};
 digit_stn2_request_t in={0};digit_stn2_result_t out={0};size_t used=0;
 assert(qualify(&q)==STNLABZ_MODULE_OK&&q.tests_passed==10);
 assert(execute(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
 assert(!authorized_command("gen intel admin"));
 assert(!authorized_command("weekly brief"));
 {response_t stream={0};char chunk[8192]={0};size_t i;
  for(i=0;i<75;i++)assert(capture(chunk,1,sizeof(chunk),&stream)==sizeof(chunk));
  assert(stream.len==614400&&!stream.exceeded);
  while(stream.len<SOURCE_LIMIT){
   size_t remaining=SOURCE_LIMIT-stream.len;
   size_t next=remaining<sizeof(chunk)?remaining:sizeof(chunk);
   assert(capture(chunk,1,next,&stream)==next);
  }
  assert(stream.len==SOURCE_LIMIT);
  assert(capture(chunk,1,1,&stream)==0&&stream.exceeded);
 }
 {
  response_t parsed={0};
  const char *p1="something CVE-2026-";
  const char *p2="12345 and CVE-2026-12345, plus CVE-2025-4321.";
  assert(capture((char *)p1,1,strlen(p1),&parsed)==strlen(p1));
  assert(capture((char *)p2,1,strlen(p2),&parsed)==strlen(p2));
  finish_scan(&parsed);
  assert(parsed.cve_mentions==3);
  assert(parsed.unique_cves==2);
  assert(strcmp(parsed.cves[0],"CVE-2026-12345")==0);
  assert(strcmp(parsed.cves[1],"CVE-2025-4321")==0);
 }
 assert(strcmp(fetch_label(FETCH_LIMIT),"SIZE_LIMIT")==0);
 assert(strcmp(fetch_label(FETCH_HTTP),"HTTP_ERROR")==0);
 assert(strcmp(fetch_label(FETCH_EMPTY),"EMPTY_RESPONSE")==0);
 assert(strcmp(fetch_label(FETCH_NETWORK),"NETWORK_ERROR")==0);
 puts("STN-2 offline qualification passed");return 0;
}
