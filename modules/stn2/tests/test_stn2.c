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
 {
  const char *sample="{\"stats\":{\"total_threats\":1394,\"by_type\":{\"pattern_match\":1195,\"wp_admin_scan\":176}},\"analysis\":{\"total_24h\":1,\"trending\":{\"probe\":1}},\"patterns\":[\"wp-admin\",\"xmlrpc\"]}";
  char output[4096]={0};
  response_t intel={0};
  assert(capture((char *)sample,1,strlen(sample),&intel)==strlen(sample));
  finish_scan(&intel);
  assert(intel_report(intel.intel_json,output,sizeof(output)));
  assert(strstr(output,"1394 observations")!=NULL);
  assert(strstr(output,"wp_admin_scan: 176")!=NULL);
  assert(strstr(output,"last 24h: 1")!=NULL);
  assert(strstr(output,"xmlrpc")!=NULL);
  output[0]=0;
  assert(!intel_report("{\"stats\":{}}",output,sizeof(output)));
 }
 {
  char dir[]="/tmp/stn2-test-XXXXXX",path[256],report[4096]={0};
  const char *one="{\"stats\":{\"total_threats\":10}}";
  const char *two="{\"stats\":{\"total_threats\":13}}";
  assert(mkdtemp(dir)!=NULL);
  assert(snprintf(path,sizeof(path),"%s/intel-state.json",dir)<(int)sizeof(path));
  assert(intel_state(one,report,sizeof(report),path));
  assert(strstr(report,"initial baseline")!=NULL);
  report[0]=0;
  assert(intel_state(two,report,sizeof(report),path));
  assert(strstr(report,"+3 historical records")!=NULL);
  assert(unlink(path)==0);
  assert(rmdir(dir)==0);
 }
 {
  char dir[]="/tmp/stn2-record-XXXXXX",path[256],report[4096]={0};
  const char *first="{\"threats\":[{\"id\":\"t_a\",\"type\":\"pattern_match\",\"created_at\":\"2026-10-10 17:00:00\"}]}";
  const char *next="{\"threats\":[{\"id\":\"t_a\"},{\"id\":\"t_b\",\"type\":\"bot_probe\",\"created_at\":\"2026-10-10 17:10:00\"},{\"id\":\"t_b\"}]}";
  assert(mkdtemp(dir)!=NULL);
  assert(snprintf(path,sizeof(path),"%s/threat-ids.json",dir)<(int)sizeof(path));
  assert(threat_records(first,report,sizeof(report),path,NULL));
  assert(strstr(report,"initial baseline of 1 records")!=NULL);
  report[0]=0;
  assert(threat_records(next,report,sizeof(report),path,NULL));
  assert(strstr(report,"2 records; 1 absent from previous snapshot")!=NULL);
  assert(strstr(report,"New record t_b type=bot_probe")!=NULL);
  assert(unlink(path)==0);
  assert(rmdir(dir)==0);
 }
 {
  char dir[]="/tmp/stn2-corr-XXXXXX",baseline[256],log[256],out[4096]={0};
  FILE *f;
  const char *a="{\"threats\":[{\"id\":\"a\"}]}";
  const char *b="{\"threats\":[{\"id\":\"a\"},{\"id\":\"b\",\"ip\":\"192.0.2.9\",\"type\":\"bot_probe\"}]}";
  assert(mkdtemp(dir)!=NULL);
  assert(snprintf(baseline,sizeof(baseline),"%s/ids.json",dir)<(int)sizeof(baseline));
  assert(snprintf(log,sizeof(log),"%s/rictus.log",dir)<(int)sizeof(log));
  f=fopen(log,"w");assert(f);
  assert(fputs("WARN probe from 192.0.2.9 at path /xmlrpc.php\n",f)>=0);
  assert(fclose(f)==0);
  {rictus_window_t window={0};
   assert(rictus_load(log,&window));
   assert(window.available&&window.count==1);
   assert(rictus_window_match(&window,"192.0.2.9")==1);
   assert(rictus_window_match(&window,"192.0.2.10")==0);
  }
  assert(same_ip_token("src=192.0.2.9","192.0.2.9"));
  assert(!same_ip_token("src=1192.0.2.9","192.0.2.9"));
  assert(!same_ip_token("src=192.0.2.90","192.0.2.9"));
  assert(threat_records(a,out,sizeof(out),baseline,log));
  out[0]=0;
  assert(threat_records(b,out,sizeof(out),baseline,log));
  assert(strstr(out,"Rictus: matching IP token")!=NULL);
  assert(strstr(out,"Historical correlation: 1 distinct records checked by IP; 1 matching Rictus IP tokens")!=NULL);
  assert(strstr(out,"Historical candidate: record=b IP=192.0.2.9")!=NULL);
  out[0]=0;
  assert(threat_records(b,out,sizeof(out),baseline,log));
  assert(strstr(out,"0 absent from previous snapshot")!=NULL);
  assert(strstr(out,"1 matching Rictus IP tokens")!=NULL);
  assert(unlink(baseline)==0);
  assert(unlink(log)==0);
  assert(rmdir(dir)==0);
 }
 assert(strcmp(fetch_label(FETCH_LIMIT),"SIZE_LIMIT")==0);
 assert(strcmp(fetch_label(FETCH_HTTP),"HTTP_ERROR")==0);
 assert(strcmp(fetch_label(FETCH_EMPTY),"EMPTY_RESPONSE")==0);
 assert(strcmp(fetch_label(FETCH_NETWORK),"NETWORK_ERROR")==0);
 puts("STN-2 offline qualification passed");return 0;
}
