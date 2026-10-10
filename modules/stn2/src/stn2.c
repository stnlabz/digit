#define _POSIX_C_SOURCE 200809L
/* [AI:GPT-6 | 2026-10-10] STN-2 bounded, read-only intelligence collection.
 * A failed source is never represented as a successful retrieval. */
#include <curl/curl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "stn2.h"
#define CONFIG_PATH "/opt/digit/auth/stn2/stn2.conf"
#define SOURCE_LIMIT (8U * 1024U * 1024U)
#define LOG_TAIL_LINES 120
typedef struct {int enabled;char log[512],base[512],advisories[512];} settings_t;
typedef struct {size_t len;int exceeded;} response_t;
typedef enum {FETCH_OK=0,FETCH_INVALID_URL,FETCH_NETWORK,FETCH_HTTP,FETCH_EMPTY,FETCH_LIMIT} fetch_status_t;
static settings_t settings;
static const stnlabz_module_host_t *owner;
static int is_private(const char *path){
 struct stat st;return path&&lstat(path,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink==1&&
   !(st.st_mode&077)&&(st.st_uid==0||st.st_uid==geteuid());
}
static int safe_copy(char *dst,size_t n,const char *src){
 size_t len;if(!src)return 0;len=strlen(src);
 if(!len||len>=n)return 0;memcpy(dst,src,len+1);return 1;
}
static int parse_settings(const char *path,settings_t *out){
 FILE *fp;char line[1024];settings_t c={0};int ok=1;
 if(!is_private(path)||!out)return 0;
 fp=fopen(path,"r");if(!fp)return 0;
 while(fgets(line,sizeof(line),fp)){
  char *eq,*key=line,*value,*end;
  if(!strchr(line,'\n')&&!feof(fp)){ok=0;break;}
  while(isspace((unsigned char)*key))key++;
  if(!*key||*key=='#')continue;
  eq=strchr(key,'=');if(!eq){ok=0;break;}*eq++=0;
  end=key+strlen(key);while(end>key&&isspace((unsigned char)end[-1]))*--end=0;
  value=eq;end=value+strlen(value);while(end>value&&isspace((unsigned char)end[-1]))*--end=0;
  if(!strcmp(key,"enabled")){if(!strcmp(value,"true"))c.enabled=1;else if(strcmp(value,"false"))ok=0;}
  else if(!strcmp(key,"rictus_log"))ok=safe_copy(c.log,sizeof(c.log),value);
  else if(!strcmp(key,"api_base"))ok=safe_copy(c.base,sizeof(c.base),value);
  else if(!strcmp(key,"advisory_list"))ok=safe_copy(c.advisories,sizeof(c.advisories),value);
  else ok=0;
  if(!ok)break;
 }
 if(ferror(fp))ok=0;fclose(fp);
 if(ok&&c.enabled)ok=c.log[0]&&c.base[0]&&strncmp(c.base,"https://",8)==0&&c.advisories[0];
 if(ok)*out=c;return ok;
}
static size_t capture(char *ptr,size_t size,size_t n,void *ctx){
 response_t *b=ctx;size_t bytes;
 (void)ptr;
 if(n&&size>SIZE_MAX/n){b->exceeded=1;return 0;}
 bytes=size*n;
 if(bytes>SOURCE_LIMIT-b->len){b->exceeded=1;return 0;}
 b->len+=bytes;return bytes;
}
static fetch_status_t https_fetch(const char *url,response_t *body,long *http_code){
 CURL *h;CURLcode result;long code=0;
 if(!url||strncmp(url,"https://",8)||!body)return FETCH_INVALID_URL;
 memset(body,0,sizeof(*body));if(http_code)*http_code=0;
 h=curl_easy_init();if(!h)return FETCH_NETWORK;
 curl_easy_setopt(h,CURLOPT_URL,url);
 curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,capture);
 curl_easy_setopt(h,CURLOPT_WRITEDATA,body);
 curl_easy_setopt(h,CURLOPT_CONNECTTIMEOUT,8L);
 curl_easy_setopt(h,CURLOPT_TIMEOUT,15L);
 curl_easy_setopt(h,CURLOPT_FOLLOWLOCATION,0L);
 curl_easy_setopt(h,CURLOPT_PROTOCOLS_STR,"https");
 result=curl_easy_perform(h);
 curl_easy_getinfo(h,CURLINFO_RESPONSE_CODE,&code);
 curl_easy_cleanup(h);
 if(http_code)*http_code=code;
 if(body->exceeded)return FETCH_LIMIT;
 if(result!=CURLE_OK)return FETCH_NETWORK;
 if(code!=200)return FETCH_HTTP;
 return body->len?FETCH_OK:FETCH_EMPTY;
}
static const char *fetch_label(fetch_status_t status){
 switch(status){
 case FETCH_OK:return "RETRIEVED";
 case FETCH_INVALID_URL:return "INVALID_URL";
 case FETCH_NETWORK:return "NETWORK_ERROR";
 case FETCH_HTTP:return "HTTP_ERROR";
 case FETCH_EMPTY:return "EMPTY_RESPONSE";
 case FETCH_LIMIT:return "SIZE_LIMIT";
 default:return "UNKNOWN_ERROR";
 }
}
static void append(char *out,size_t size,const char *line){
 size_t used=strlen(out),n=strlen(line);if(used+n<size-1)memcpy(out+used,line,n+1);
}
static void add_source_result(digit_stn2_result_t *r,const char *name,
                              fetch_status_t status,size_t bytes,long http){
 char line[256];
 snprintf(line,sizeof(line),"%-18s %s (%zu bytes; HTTP %ld)\n",
          name,fetch_label(status),bytes,http);
 if(status==FETCH_OK)r->sources_ok++;else r->sources_failed++;
 append(r->report,sizeof(r->report),line);
}
static int rictus_summary(const char *path,char *summary,size_t cap){
 FILE *f;char line[2048],last[LOG_TAIL_LINES][512];size_t n=0,i,begin;
 struct stat st;unsigned int count=0;
 if(lstat(path,&st)!=0||!S_ISREG(st.st_mode)||st.st_nlink!=1)return 0;
 f=fopen(path,"r");if(!f)return 0;
 while(fgets(line,sizeof(line),f)){
  size_t length=strlen(line);
  if(!strchr(line,'\n')&&!feof(f)){int ch;while((ch=fgetc(f))!='\n'&&ch!=EOF){}continue;}
  if(length>=sizeof(last[0]))length=sizeof(last[0])-1;
  memcpy(last[n%LOG_TAIL_LINES],line,length);
  last[n%LOG_TAIL_LINES][length]=0;n++;
 }
 if(ferror(f)){fclose(f);return 0;}fclose(f);
 begin=n>LOG_TAIL_LINES?n-LOG_TAIL_LINES:0;
 for(i=begin;i<n;i++){
  const char *v=last[i%LOG_TAIL_LINES];
  if(strstr(v,"ERROR")||strstr(v,"WARN")||strstr(v,"ALERT"))count++;
 }
 snprintf(summary,cap,"Rictus: inspected last %zu records, %u flagged (WARN/ERROR/ALERT text matches).\n",
 n<LOG_TAIL_LINES?n:(size_t)LOG_TAIL_LINES,count);
 return 1;
}
static int authorized_command(const char *s){
 return s&&(!strcmp(s,"gen intel")||!strcmp(s,"generate feed"));
}
static stnlabz_module_result_t execute(const void *request,size_t req_size,void *output,size_t output_size,size_t *used,void *ctx){
 static const char *const endpoints[]={"/threats","/events","/intel","/patterns","/rss"};
 const digit_stn2_request_t *in=request;digit_stn2_result_t r={0};response_t *buf;size_t i;long http=0;fetch_status_t status;
 FILE *list;char url[1024],line[1024],log_summary[256];
 (void)ctx;
 if(!owner||!in||req_size!=sizeof(*in)||!output||output_size<sizeof(r)||!used||
    !memchr(in->command,0,sizeof(in->command))||!memchr(in->actor,0,sizeof(in->actor))||
    !authorized_command(in->command)||!in->actor[0])
  return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 buf=calloc(1,sizeof(*buf));if(!buf)return STNLABZ_MODULE_ERR_START_FAILED;
 append(r.report,sizeof(r.report),"STN-2 Intelligence Collection\nSource status (read-only):\n");
 if(rictus_summary(settings.log,log_summary,sizeof(log_summary))){
  r.sources_ok++;append(r.report,sizeof(r.report),log_summary);
 }else add_source_result(&r,"Rictus",FETCH_NETWORK,0,0);
 for(i=0;i<sizeof(endpoints)/sizeof(endpoints[0]);i++){
  if(snprintf(url,sizeof(url),"%s%s",settings.base,endpoints[i])>=(int)sizeof(url))
   add_source_result(&r,endpoints[i],FETCH_INVALID_URL,0,0);
  else {status=https_fetch(url,buf,&http);add_source_result(&r,endpoints[i],status,buf->len,http);}
 }
 list=NULL;
 if(is_private(settings.advisories))list=fopen(settings.advisories,"r");
 if(!list)append(r.report,sizeof(r.report),"Advisory sources: NOT CONFIGURED/UNAVAILABLE\n");
 else{
  unsigned int checked=0;
  while(checked<32&&fgets(line,sizeof(line),list)){
   char label[40];size_t len;
   if(!strchr(line,'\n')&&!feof(list))break;
   len=strcspn(line,"\r\n");line[len]=0;
   if(!*line||line[0]=='#')continue;
   snprintf(label,sizeof(label),"Advisory %u",checked+1);
   status=https_fetch(line,buf,&http);add_source_result(&r,label,status,buf->len,http);checked++;
  }
  fclose(list);
 }
 append(r.report,sizeof(r.report),
  "Note: retrieval is not analysis or CVE verification. No unsupported threat claims made.\n");
 free(buf);memcpy(output,&r,sizeof(r));*used=sizeof(r);
 return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t qualify(stnlabz_module_qualification_result_t *q){
 unsigned int passed=0;settings_t s={0};char text[32];
 if(!q)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(q,0,sizeof(*q));
 passed+=authorized_command("gen intel");
 passed+=authorized_command("generate feed");
 passed+=!authorized_command("weekly brief");
 passed+=!authorized_command("gen intel cms");
 passed+=!authorized_command("gen intel ");
 passed+=!authorized_command(NULL);
 passed+=safe_copy(text,sizeof(text),"valid");
 passed+=!safe_copy(text,3,"overflow");
 passed+=!is_private("/nonexistent/stn2/config");
 passed+=!parse_settings("/nonexistent/stn2/config",&s);
 q->tests_executed=10;q->tests_passed=passed;q->tests_failed=10-passed;
 q->negative_test_executed=1;q->negative_test_passed=!parse_settings("/nonexistent/stn2/config",&s);
 return q->tests_failed?STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t start(const stnlabz_module_host_t *h){
 if(!h||!h->register_service||!h->unregister_service)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 if(!parse_settings(CONFIG_PATH,&settings)||!settings.enabled)return STNLABZ_MODULE_ERR_START_FAILED;
 if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return STNLABZ_MODULE_ERR_START_FAILED;
 if(!h->register_service(DIGIT_STN2_SERVICE,execute,NULL)){
  curl_global_cleanup();return STNLABZ_MODULE_ERR_START_FAILED;
 }
 owner=h;return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t stop(void){
 if(owner&&owner->unregister_service)
  (void)owner->unregister_service(DIGIT_STN2_SERVICE,NULL);
 owner=NULL;memset(&settings,0,sizeof(settings));curl_global_cleanup();return STNLABZ_MODULE_OK;
}
static const stnlabz_module_descriptor_t descriptor={
 "stn2","Digit STN-2 Intelligence",1,0,1,STNLABZ_MODULE_API_MAJOR,
 STNLABZ_MODULE_API_MINOR,qualify,start,stop
};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &descriptor;}
