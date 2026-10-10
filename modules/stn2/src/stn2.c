#define _POSIX_C_SOURCE 200809L
/* [AI:GPT-6 | 2026-10-10] STN-2 bounded, read-only intelligence collection.
 * A failed source is never represented as a successful retrieval. */
#include <curl/curl.h>
#include <json-c/json.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "stn2.h"
#define CONFIG_PATH "/opt/digit/auth/stn2/stn2.conf"
#define STATE_PATH "/opt/digit/auth/stn2/intel-state.json"
#define SOURCE_LIMIT (8U * 1024U * 1024U)
#define LOG_TAIL_LINES 120
typedef struct {int enabled;char log[512],base[512],advisories[512];} settings_t;
typedef struct {
 size_t len;
 int exceeded;
 char window[32];
 size_t window_len;
 char intel_json[4096];
 size_t intel_len;
 char cves[8][24];
 unsigned int unique_cves;
 unsigned int cve_mentions;
} response_t;
typedef enum {FETCH_OK=0,FETCH_INVALID_URL,FETCH_NETWORK,FETCH_HTTP,FETCH_EMPTY,FETCH_LIMIT} fetch_status_t;
static settings_t settings;
static const stnlabz_module_host_t *owner;
static int is_private(const char *path){
 struct stat st;return path&&lstat(path,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink==1&&
   !(st.st_mode&077)&&(st.st_uid==0||st.st_uid==geteuid());
}
static int safe_copy(char *dst,size_t n,const char *src){
 size_t len;if(!src)return 0;len=strlen(src);
 if(!len||len>=n)return 0;
 memcpy(dst,src,len+1);
 return 1;
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
 if(ferror(fp))ok=0;
 fclose(fp);
 if(ok&&c.enabled)ok=c.log[0]&&c.base[0]&&strncmp(c.base,"https://",8)==0&&c.advisories[0];
 if(ok)*out=c;
 return ok;
}
static int cve_token(const char *p,size_t n){
 size_t i;
 if(n<13||n>22||memcmp(p,"CVE-",4)!=0||p[8]!='-')return 0;
 for(i=4;i<8;i++)if(!isdigit((unsigned char)p[i]))return 0;
 for(i=9;i<n;i++)if(!isdigit((unsigned char)p[i]))return 0;
 return 1;
}
static void cve_track(response_t *b,const char *token,size_t n){
 unsigned int i;
 if(!cve_token(token,n))return;
 b->cve_mentions++;
 for(i=0;i<b->unique_cves;i++)
  if(strlen(b->cves[i])==n&&!memcmp(b->cves[i],token,n))return;
 if(b->unique_cves<8){
  memcpy(b->cves[b->unique_cves],token,n);
  b->cves[b->unique_cves][n]=0;
  b->unique_cves++;
 }
}
static void scan_bytes(response_t *b,const char *data,size_t length){
 static const char prefix[]="CVE-";
 size_t i,n;
 for(i=0;i<length;i++){
  unsigned char ch=(unsigned char)data[i];
  n=b->window_len;
  if(n==0){if(ch=='C'){b->window[0]='C';b->window_len=1;}continue;}
  if(n<4&&ch==(unsigned char)prefix[n]){
   b->window[n]=(char)ch;b->window_len=n+1;continue;
  }
  if((n>=4&&n<8&&isdigit(ch))||(n==8&&ch=='-')||
     (n>=9&&n<22&&isdigit(ch))){
   b->window[n]=(char)ch;b->window_len=n+1;continue;
  }
  cve_track(b,b->window,n);
  b->window_len=0;
  if(ch=='C'){b->window[0]='C';b->window_len=1;}
 }
}
static void finish_scan(response_t *b){
 if(b->window_len)cve_track(b,b->window,b->window_len);
 b->window_len=0;
}
static size_t capture(char *ptr,size_t size,size_t n,void *ctx){
 response_t *b=ctx;size_t bytes;
 if(n&&size>SIZE_MAX/n){b->exceeded=1;return 0;}
 bytes=size*n;
 if(bytes>SOURCE_LIMIT-b->len){b->exceeded=1;return 0;}
 scan_bytes(b,ptr,bytes);
 if(b->intel_len+bytes<sizeof(b->intel_json)){
  memcpy(b->intel_json+b->intel_len,ptr,bytes);
  b->intel_len+=bytes;
  b->intel_json[b->intel_len]=0;
 }else b->intel_len=sizeof(b->intel_json);
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
 if(result==CURLE_OK)finish_scan(body);
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
/* /intel carries aggregate statistics, not verified incidents.
 * This parser declines malformed or unexpected representations rather than
 * creating plausible-looking numbers. */
static int json_u64(struct json_object *obj,const char *key,int64_t *value){
 struct json_object *member=NULL;
 if(!obj||!json_object_object_get_ex(obj,key,&member)||
    !json_object_is_type(member,json_type_int))return 0;
 *value=json_object_get_int64(member);
 return *value>=0;
}
static struct json_object *json_member_object(struct json_object *obj,const char *key){
 struct json_object *member=NULL;
 if(!obj||!json_object_object_get_ex(obj,key,&member)||
    !json_object_is_type(member,json_type_object))return NULL;
 return member;
}
static int intel_report(const char *raw,char *out,size_t capacity){
 struct json_object *root,*stats,*types,*analysis,*trending,*patterns;
 int64_t total,day=0;char line[256];
 unsigned int count=0;
 if(!raw||!out||!capacity)return 0;
 root=json_tokener_parse(raw);
 if(!root||!json_object_is_type(root,json_type_object)){
  if(root)json_object_put(root);
  return 0;
 }
 stats=json_member_object(root,"stats");
 analysis=json_member_object(root,"analysis");
 types=json_member_object(stats,"by_type");
 if(!json_u64(stats,"total_threats",&total)||!types){json_object_put(root);return 0;}
 snprintf(line,sizeof(line),"Sentinel assessment (reported historical total): %lld observations\n",(long long)total);
 append(out,capacity,line);
 json_object_object_foreach(types,key,val){
  int64_t number;
  if(!json_object_is_type(val,json_type_int))continue;
  number=json_object_get_int64(val);
  if(number<0)continue;
  snprintf(line,sizeof(line),"  %.*s: %lld\n",64,key,(long long)number);
  append(out,capacity,line);
  count++;
 }
 if(!count){json_object_put(root);return 0;}
 if(json_u64(analysis,"total_24h",&day)){
  snprintf(line,sizeof(line),"Sentinel reported last 24h: %lld observations\n",(long long)day);
  append(out,capacity,line);
 }
 trending=json_member_object(analysis,"trending");
 if(trending){
  json_object_object_foreach(trending,key,val){
   if(json_object_is_type(val,json_type_int)&&json_object_get_int64(val)>=0){
    snprintf(line,sizeof(line),"  24h trend %.*s: %lld\n",64,key,
             (long long)json_object_get_int64(val));
    append(out,capacity,line);
   }
  }
 }
 patterns=NULL;
 if(json_object_object_get_ex(root,"patterns",&patterns)&&
    json_object_is_type(patterns,json_type_array)){
  int i,limit=json_object_array_length(patterns);
  if(limit>12)limit=12;
  append(out,capacity,"Reported pattern indicators (not proof of compromise):\n");
  for(i=0;i<limit;i++){
   struct json_object *item=json_object_array_get_idx(patterns,i);
   const char *value=json_object_is_type(item,json_type_string)?
                      json_object_get_string(item):NULL;
   if(value&&strlen(value)<80&&strchr(value,'\n')==NULL){
    snprintf(line,sizeof(line),"  %s\n",value);
    append(out,capacity,line);
   }
  }
 }
 json_object_put(root);
 return 1;
}
/* The aggregate state is a baseline, not an evidence store or case record.
 * Only valid /intel snapshots advance it. Security failures are reported.
 * Atomic rename avoids torn writes; a prior private file must be owned safely. */
static int intel_state(const char *raw,char *out,size_t cap,const char *path){
 struct json_object *root=NULL,*stats=NULL,*old=NULL,*oldstats=NULL;
 int64_t current=0,previous=0;
 char line[256],tmp[600];
 FILE *fp=NULL;
 int fd=-1,ok=0;
 struct stat st;
 if(!raw||!out||!path)return 0;
 root=json_tokener_parse(raw);
 if(!root||!json_object_is_type(root,json_type_object))goto done;
 stats=json_member_object(root,"stats");
 if(!json_u64(stats,"total_threats",&current))goto done;
 if(lstat(path,&st)==0){
  if(!is_private(path)){append(out,cap,"Change tracking: UNSAFE STATE FILE\n");goto done;}
  old=json_object_from_file(path);
  if(!old){append(out,cap,"Change tracking: CORRUPT PRIOR STATE\n");goto done;}
  oldstats=json_member_object(old,"stats");
  if(!json_u64(oldstats,"total_threats",&previous)){
   append(out,cap,"Change tracking: INVALID PRIOR BASELINE\n");goto done;
  }
  if(current>=previous)
   snprintf(line,sizeof(line),"Change since prior collection: +%lld historical records (aggregate only)\n",
           (long long)(current-previous));
  else snprintf(line,sizeof(line),"Historical total decreased by %lld; check reset or retention policy\n",
                (long long)(previous-current));
 }else if(errno==ENOENT){
  snprintf(line,sizeof(line),"Change tracking: initial baseline established (%lld records)\n",(long long)current);
 }else{
  append(out,cap,"Change tracking: STATE ACCESS ERROR\n");goto done;
 }
 if(snprintf(tmp,sizeof(tmp),"%s.tmp.%ld",path,(long)getpid())>=(int)sizeof(tmp))goto done;
 fd=open(tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(fd<0){append(out,cap,"Change tracking: STATE WRITE FAILED\n");goto done;}
 fp=fdopen(fd,"w");
 if(!fp){close(fd);fd=-1;unlink(tmp);goto done;}
 fd=-1;
 if(fputs(raw,fp)==EOF||fflush(fp)!=0||fsync(fileno(fp))!=0||fclose(fp)!=0){
  fp=NULL;unlink(tmp);append(out,cap,"Change tracking: STATE WRITE FAILED\n");goto done;
 }
 fp=NULL;
 if(rename(tmp,path)!=0){unlink(tmp);append(out,cap,"Change tracking: STATE RENAME FAILED\n");goto done;}
 append(out,cap,line);
 ok=1;
 done:
 if(fp)fclose(fp);
 if(fd>=0)close(fd);
 if(old)json_object_put(old);
 if(root)json_object_put(root);
 return ok;
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
  else {
   unsigned int j;
   char finding[160];
   status=https_fetch(url,buf,&http);
   add_source_result(&r,endpoints[i],status,buf->len,http);
   if(status==FETCH_OK&&strcmp(endpoints[i],"/intel")==0){
    if(buf->intel_len<sizeof(buf->intel_json)&&
       intel_report(buf->intel_json,r.report,sizeof(r.report))){
      (void)intel_state(buf->intel_json,r.report,sizeof(r.report),STATE_PATH);
      append(r.report,sizeof(r.report),
       "Assessment: reported web probing is not evidence of successful compromise.\n");
    }else append(r.report,sizeof(r.report),"Sentinel /intel schema: UNPARSEABLE\n");
   }
   if(status==FETCH_OK){
    snprintf(finding,sizeof(finding),"  CVE references detected: %u mentions; up to %u distinct identifiers sampled\n",
             buf->cve_mentions,buf->unique_cves);
    append(r.report,sizeof(r.report),finding);
    for(j=0;j<buf->unique_cves;j++){
     snprintf(finding,sizeof(finding),"    %s (unverified textual reference)\n",buf->cves[j]);
     append(r.report,sizeof(r.report),finding);
    }
   }
  }
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
 if(!q)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 memset(q,0,sizeof(*q));
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
 "stn2","Digit STN-2 Intelligence",1,0,6,STNLABZ_MODULE_API_MAJOR,
 STNLABZ_MODULE_API_MINOR,qualify,start,stop
};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &descriptor;}
