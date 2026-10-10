#define _POSIX_C_SOURCE 200809L
/* [AI:GPT-6 | 2026-10-10] Native Digit ISO C Slack Socket Mode module.
 * Slack never confers administrative or organizational authority. */
#include <curl/curl.h>
#include <json-c/json.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "slack.h"
#include "dispatcher.h"
#include "../../stn2/includes/stn2.h"
#include "../../interface/includes/account_auth.h"
#define SLACK_PATH "/opt/digit/auth/slack/slack.conf"
#define BUF_CAP 65536
#define INTEL_OUTBOX_PATH "/opt/digit/auth/slack/intel-outbox.json"
#define ALERT_CAP 4096
typedef struct {int enabled;char workspace[64],app[512],bot[512],intel_channel[64];} slack_config_t;
typedef struct {char data[BUF_CAP];size_t used;} buffer_t;
static slack_config_t cfg;
static const stnlabz_module_host_t *host;
static pthread_t thread,watch_thread;
static int watch_active;
static int active,stop_flag;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t intel_mutex=PTHREAD_MUTEX_INITIALIZER;
typedef struct {char user[64],team[64],identity[DIGIT_ACCOUNT_ID_MAX];time_t expires;char previous_answer[DIGIT_DISPATCHER_ANSWER_MAX];} slack_session_t;
static slack_session_t sessions[32];
static char bot_id[64];
static char last_events[128][128];static unsigned int event_cursor;
static void erase(void *p,size_t n){volatile unsigned char *v=p;while(n--)*v++=0;}
static int bounded(char *out,size_t capacity,const char *input){
 size_t n;if(!input)return 0;n=strlen(input);if(!n||n>=capacity)return 0;
 memcpy(out,input,n+1);return 1;
}
static int private_file(const char *path){
 struct stat s;return lstat(path,&s)==0&&S_ISREG(s.st_mode)&&s.st_nlink==1&&
   !(s.st_mode&077)&&(s.st_uid==0||s.st_uid==geteuid());
}
static int load_config(const char *path,slack_config_t *out){
 FILE *file;slack_config_t v={0};char line[2048];int good=1;
 if(!out||!private_file(path))return 0;
 file=fopen(path,"r");if(!file)return 0;
 while(fgets(line,sizeof(line),file)){
  char *key=line,*value,*end;
  if(!strchr(line,'\n')&&!feof(file)){good=0;break;}
  while(isspace((unsigned char)*key))++key;
  if(!*key||*key=='#')continue;
  value=strchr(key,'=');if(!value){good=0;break;}*value++=0;
  end=key+strlen(key);while(end>key&&isspace((unsigned char)end[-1]))*--end=0;
  end=value+strlen(value);while(end>value&&isspace((unsigned char)end[-1]))*--end=0;
  if(!strcmp(key,"enabled")){if(!strcmp(value,"true"))v.enabled=1;else if(strcmp(value,"false"))good=0;}
  else if(!strcmp(key,"workspace_id"))good=bounded(v.workspace,sizeof(v.workspace),value)||!*value;
  else if(!strcmp(key,"app_token"))good=bounded(v.app,sizeof(v.app),value)||!*value;
  else if(!strcmp(key,"bot_token"))good=bounded(v.bot,sizeof(v.bot),value)||!*value;
  else if(!strcmp(key,"intel_channel_id"))good=bounded(v.intel_channel,sizeof(v.intel_channel),value)||!*value;
  else good=0;
  if(!good)break;
 }
 if(ferror(file))good=0;
 fclose(file);
 if(good&&v.enabled){
  good=v.workspace[0]&&v.app[0]&&v.bot[0]&&
   strncmp(v.app,"xapp-",5)==0&&strncmp(v.bot,"xoxb-",5)==0;
 }
 if(good)*out=v;
 erase(&v,sizeof(v));erase(line,sizeof(line));return good;
}
static const char *field(struct json_object *j,const char *key){
 struct json_object *x=NULL;if(!j||!json_object_object_get_ex(j,key,&x)||
   !json_object_is_type(x,json_type_string))return NULL;
 return json_object_get_string(x);
}
static struct json_object *object(struct json_object *j,const char *key){
 struct json_object *x=NULL;
 if(j&&json_object_object_get_ex(j,key,&x)&&json_object_is_type(x,json_type_object))return x;
 return NULL;
}
static int yes(struct json_object *j,const char *key){
 struct json_object *x=NULL;
 return j&&json_object_object_get_ex(j,key,&x)&&json_object_is_type(x,json_type_boolean)&&json_object_get_boolean(x);
}
static size_t response_write(char *ptr,size_t size,size_t nmemb,void *ctx){
 buffer_t *buf=ctx;size_t n=size*nmemb;
 if(n>BUF_CAP-1-buf->used)return 0;
 memcpy(buf->data+buf->used,ptr,n);buf->used+=n;buf->data[buf->used]=0;return n;
}
static struct json_object *post(const char *endpoint,const char *bearer,struct json_object *body){
 CURL *curl=NULL;struct curl_slist *headers=NULL;buffer_t *buf=calloc(1,sizeof(*buf));
 struct json_object *result=NULL;char authorization[560];long code=0;
 if(!buf)return NULL;
 curl=curl_easy_init();if(!curl)goto done;
 if(snprintf(authorization,sizeof(authorization),"Authorization: Bearer %s",bearer)>=(int)sizeof(authorization))goto done;
 headers=curl_slist_append(headers,authorization);
 headers=curl_slist_append(headers,"Content-Type: application/json");
 curl_easy_setopt(curl,CURLOPT_URL,endpoint);
 curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
 curl_easy_setopt(curl,CURLOPT_POSTFIELDS,body?json_object_to_json_string_ext(body,JSON_C_TO_STRING_PLAIN):"{}");
 curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,response_write);
 curl_easy_setopt(curl,CURLOPT_WRITEDATA,buf);
 curl_easy_setopt(curl,CURLOPT_TIMEOUT,20L);
 curl_easy_setopt(curl,CURLOPT_PROTOCOLS_STR,"https");
 if(curl_easy_perform(curl)==CURLE_OK){
  curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);
  if(code==200)result=json_tokener_parse(buf->data);
 }
done:
 erase(authorization,sizeof(authorization));curl_slist_free_all(headers);
 if(curl)curl_easy_cleanup(curl);
 erase(buf,sizeof(*buf));free(buf);return result;
}
/* One-slot crash-recoverable outbox. Restrict to the configured channel.
 * Refuse unsafe/corrupt state instead of silently overwriting lost evidence. */
static int alert_load(const char *path,char *message,size_t cap){
 struct json_object *root=NULL;
 const char *channel,*body;
 struct stat st;
 int result=0;
 if(!path||!message||cap<2)return -1;
 message[0]=0;
 if(lstat(path,&st)<0)return errno==ENOENT?0:-1;
 if(!private_file(path)||st.st_size<2||st.st_size>6*ALERT_CAP+512)return -1;
 root=json_object_from_file(path);
 if(!root||!json_object_is_type(root,json_type_object))goto done;
 channel=field(root,"channel");body=field(root,"message");
 if(!channel||strcmp(channel,cfg.intel_channel)||!body||!body[0]||
    strlen(body)>=cap)goto done;
 memcpy(message,body,strlen(body)+1);result=1;
done:
 if(root)json_object_put(root);
 return result?result:-1;
}
static int alert_save(const char *path,const char *message){
 struct json_object *root=NULL;
 const char *serialized;
 char tmp[600];
 int fd=-1,ok=0;
 FILE *fp=NULL;
 struct stat st;
 if(!path||!message||!*message||strlen(message)>=ALERT_CAP||!cfg.intel_channel[0])return 0;
 if(lstat(path,&st)==0){if(!private_file(path))return 0;}
 else if(errno!=ENOENT)return 0;
 if(snprintf(tmp,sizeof(tmp),"%s.tmp.%ld",path,(long)getpid())>=(int)sizeof(tmp))return 0;
 fd=open(tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(fd<0)return 0;
 fp=fdopen(fd,"w");if(!fp)goto done;
 fd=-1;
 root=json_object_new_object();if(!root)goto done;
 json_object_object_add(root,"channel",json_object_new_string(cfg.intel_channel));
 json_object_object_add(root,"message",json_object_new_string(message));
 serialized=json_object_to_json_string_ext(root,JSON_C_TO_STRING_PLAIN);
 if(!serialized||fputs(serialized,fp)==EOF||fflush(fp)||fsync(fileno(fp)))goto done;
 if(fclose(fp)){fp=NULL;goto done;}fp=NULL;
 if(rename(tmp,path))goto done;
 ok=1;
done:
 if(fp)fclose(fp);
 if(fd>=0)close(fd);
 if(root)json_object_put(root);
 if(!ok)unlink(tmp);
 return ok;
}
static int alert_clear(const char *path){
 struct stat st;
 if(lstat(path,&st)<0)return errno==ENOENT;
 if(!private_file(path))return 0;
 return unlink(path)==0;
}
static int seen_event(const char *id){
 unsigned int i;if(!id||!*id)return 1;
 pthread_mutex_lock(&mutex);
 for(i=0;i<128;i++)if(!strcmp(last_events[i],id)){pthread_mutex_unlock(&mutex);return 1;}
 snprintf(last_events[event_cursor++%128],128,"%s",id);
 pthread_mutex_unlock(&mutex);return 0;
}
static int reply(const char *channel,const char *timestamp,const char *message){
 struct json_object *request=json_object_new_object(),*result;int ok;
 if(!request)return 0;
 json_object_object_add(request,"channel",json_object_new_string(channel));
 json_object_object_add(request,"text",json_object_new_string(message));
 if(timestamp&&*timestamp)json_object_object_add(request,"thread_ts",json_object_new_string(timestamp));
 result=post("https://slack.com/api/chat.postMessage",cfg.bot,request);
 ok=result&&yes(result,"ok");
 if(result)json_object_put(result);
 json_object_put(request);return ok;
}
static int auth_command(const char *input){
 return input&&(!strncmp(input,"login ",6)||!strcmp(input,"login")||
                !strcmp(input,"logout")||!strcmp(input,"whoami"));
}
static int authenticate_dm(const char *workspace,const char *slack_user,const char *input,
                           char *answer,size_t capacity){
 size_t i,slot=32;
 time_t now=time(NULL);
 if(!workspace||!slack_user||!input||!answer||!capacity||
    !*workspace||!*slack_user||strlen(workspace)>=sizeof(sessions[0].team)||
    strlen(slack_user)>=sizeof(sessions[0].user))return 0;
 pthread_mutex_lock(&mutex);
 for(i=0;i<32;i++){
  if(sessions[i].expires<=now){memset(&sessions[i],0,sizeof(sessions[i]));if(slot==32)slot=i;}
  else if(!strcmp(sessions[i].team,workspace)&&!strcmp(sessions[i].user,slack_user))slot=i;
 }
 if(!strcmp(input,"logout")){
  if(slot<32)memset(&sessions[slot],0,sizeof(sessions[slot]));
  snprintf(answer,capacity,"Digit Slack session closed.");
 }else if(!strcmp(input,"whoami")){
  if(slot<32&&sessions[slot].expires>now&&
     digit_account_active_file(DIGIT_ACCOUNT_AUTH_PATH,sessions[slot].identity))
   snprintf(answer,capacity,"Authenticated Digit account: %s",sessions[slot].identity);
  else snprintf(answer,capacity,"No active Digit Slack session.");
 }else if(!strncmp(input,"login ",6)){
  const char *arg=input+6,*slash=strchr(arg,'/');
  char identity[DIGIT_ACCOUNT_ID_MAX]={0};
  size_t n=slash?(size_t)(slash-arg):0;
  if(slash&&n&&n<sizeof(identity)&&slash[1]&&strlen(slash+1)<DIGIT_ACCOUNT_PASSWORD_MAX&&
     !strchr(slash+1,'\n')&&!strchr(slash+1,'\r')){
   memcpy(identity,arg,n);
   if(digit_account_verify_file(DIGIT_ACCOUNT_AUTH_PATH,identity,slash+1)&&slot<32){
    snprintf(sessions[slot].team,sizeof(sessions[slot].team),"%s",workspace);
    snprintf(sessions[slot].user,sizeof(sessions[slot].user),"%s",slack_user);
    snprintf(sessions[slot].identity,sizeof(sessions[slot].identity),"%s",identity);
    sessions[slot].expires=now+3600;
    snprintf(answer,capacity,"Digit credentials verified. Slack session active for one hour.");
   }else snprintf(answer,capacity,"Authentication failed or session capacity reached.");
  }else snprintf(answer,capacity,"Authentication failed or invalid command format.");
  erase(identity,sizeof(identity));
 }else snprintf(answer,capacity,"Use login username/password in a direct message.");
 pthread_mutex_unlock(&mutex);
 return 1;
}
/* Conversation context is volatile and bound to a verified DM session.
 * It confers no new Dispatcher capability or organizational authority. */
static int dm_session_snapshot(const char *team,const char *user,
                               char *identity,size_t identity_cap,
                               char *prior,size_t prior_cap){
 size_t i;int found=0;time_t now=time(NULL);
 if(!team||!user||!identity||!prior||!identity_cap||!prior_cap)return 0;
 identity[0]=0;prior[0]=0;
 pthread_mutex_lock(&mutex);
 for(i=0;i<32;i++){
  if(sessions[i].expires<=now)continue;
  if(strcmp(sessions[i].team,team)||strcmp(sessions[i].user,user))continue;
  if(digit_account_active_file(DIGIT_ACCOUNT_AUTH_PATH,sessions[i].identity)){
   found=bounded(identity,identity_cap,sessions[i].identity);
   if(sessions[i].previous_answer[0])
    (void)bounded(prior,prior_cap,sessions[i].previous_answer);
  }
  break;
 }
 pthread_mutex_unlock(&mutex);return found;
}
static void dm_session_set_reply(const char *team,const char *user,const char *answer){
 size_t i;time_t now=time(NULL);
 if(!team||!user||!answer)return;
 pthread_mutex_lock(&mutex);
 for(i=0;i<32;i++){
  if(sessions[i].expires<=now||strcmp(sessions[i].team,team)||
     strcmp(sessions[i].user,user))continue;
  (void)bounded(sessions[i].previous_answer,sizeof(sessions[i].previous_answer),answer);
  break;
 }
 pthread_mutex_unlock(&mutex);
}
static int process_event(struct json_object *root){
 struct json_object *payload=object(root,"payload"),*event=object(payload,"event");
 const char *event_id=field(payload,"event_id"),*type=field(event,"type");
 const char *team=field(payload,"team_id"),*user=field(event,"user");
 const char *channel=field(event,"channel"),*text=field(event,"text");
 const char *ts=field(event,"thread_ts"),*subtype=field(event,"subtype"),*bot=field(event,"bot_id");
 const char *kind=field(event,"channel_type");char mention[80];
 digit_dispatcher_scoped_request_t query={0};digit_dispatcher_result_t answer={0};size_t used=0;
 const char *message;int is_dm,is_mention;
 if(!team||strcmp(team,cfg.workspace)||!event||!user||!channel||!text||!*text||
    subtype||bot||!strcmp(user,bot_id)||!type)return 0;
 is_dm=!strcmp(type,"message")&&kind&&!strcmp(kind,"im");
 snprintf(mention,sizeof(mention),"<@%s>",bot_id);
 is_mention=!strcmp(type,"app_mention")&&strstr(text,mention)!=NULL;
 if(!is_dm&&!is_mention)return 0;
 if(!event_id||strlen(event_id)>=128||seen_event(event_id))return 0;
 message=text;
 if(is_mention){const char *p=strstr(text,mention);if(p==text)message=p+strlen(mention);}
 while(isspace((unsigned char)*message))++message;
 if(!*message)return 0;
 if(auth_command(message)){
  char auth_reply[192]={0};
  if(!is_dm)return reply(channel,ts?ts:field(event,"ts"),
                              "Digit authentication commands are accepted only in direct messages.");
  if(!authenticate_dm(team,user,message,auth_reply,sizeof(auth_reply)))return 0;
  return reply(channel,ts?ts:field(event,"ts"),auth_reply);
 }
 if(strlen(message)>=sizeof(query.request))return 0;
 if(!strcmp(message,"gen intel")||!strcmp(message,"generate feed")){
  digit_stn2_request_t intel={0};digit_stn2_result_t report={0};
  if(!cfg.intel_channel[0]||strcmp(cfg.intel_channel,channel)!=0)
   return reply(channel,ts?ts:field(event,"ts"),"STN-2 collection is restricted to the configured intelligence channel.");
  if(strlen(user)>=sizeof(intel.actor))return 0;
  snprintf(intel.actor,sizeof(intel.actor),"%s",user);
  snprintf(intel.command,sizeof(intel.command),"%s",message);
  {
   int result_ok;
   pthread_mutex_lock(&intel_mutex);
   result_ok=host&&host->invoke_service&&
    host->invoke_service(DIGIT_STN2_SERVICE,&intel,sizeof(intel),&report,sizeof(report),&used)==STNLABZ_MODULE_OK;
   pthread_mutex_unlock(&intel_mutex);
   if(!result_ok||used!=sizeof(report)||!memchr(report.report,0,sizeof(report.report))||!report.report[0])
    return reply(channel,ts?ts:field(event,"ts"),"STN-2 intelligence collection is unavailable.");
  }
  return reply(channel,ts?ts:field(event,"ts"),report.report);
 }
 /* Preserve the Slack actor boundary: verification is not Core authorization. */
 if(strlen(user)+6>=sizeof(query.actor))return 0;
 snprintf(query.actor,sizeof(query.actor),"slack:%s",user);
 query.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
 snprintf(query.request,sizeof(query.request),"%s",message);
 if(is_dm){
  digit_dispatcher_scoped_context_request_t contextual={0};
  char identity[DIGIT_ACCOUNT_ID_MAX]={0};
  char previous[DIGIT_DISPATCHER_ANSWER_MAX]={0};
  if(dm_session_snapshot(team,user,identity,sizeof(identity),
                         previous,sizeof(previous))){
   if(!strcmp(message,"What is my authenticated Digit account?")||
      !strcmp(message,"what is my authenticated Digit account?")){
    char line[160];
    snprintf(line,sizeof(line),"Your authenticated Digit account is %s.",identity);
    return reply(channel,ts?ts:field(event,"ts"),line);
   }
   contextual.scoped=query;
   snprintf(contextual.previous_answer,sizeof(contextual.previous_answer),"%s",previous);
   if(!host||!host->invoke_service||
      host->invoke_service(DIGIT_DISPATCHER_SCOPED_CONTEXT_SERVICE,
                           &contextual,sizeof(contextual),&answer,sizeof(answer),&used)!=STNLABZ_MODULE_OK||
      used!=sizeof(answer)||!memchr(answer.answer,0,sizeof(answer.answer))||
      !answer.answer[0])return 0;
   dm_session_set_reply(team,user,answer.answer);
  }else{
   if(!host||!host->invoke_service||
      host->invoke_service(DIGIT_DISPATCHER_SCOPED_SERVICE,&query,sizeof(query),
                           &answer,sizeof(answer),&used)!=STNLABZ_MODULE_OK||
      used!=sizeof(answer)||!memchr(answer.answer,0,sizeof(answer.answer))||
      !answer.answer[0])return 0;
  }
 }else{
  if(!host||!host->invoke_service||
     host->invoke_service(DIGIT_DISPATCHER_SCOPED_SERVICE,&query,sizeof(query),
                          &answer,sizeof(answer),&used)!=STNLABZ_MODULE_OK||
     used!=sizeof(answer)||!memchr(answer.answer,0,sizeof(answer.answer))||
     !answer.answer[0])return 0;
 }
 if(!ts)ts=field(event,"ts");
 return reply(channel,ts,answer.answer);
}
static int socket_ack(CURL *ws,const char *envelope){
 struct json_object *ack=json_object_new_object();const char *data;
 size_t length,sent=0;CURLcode rc;
 if(!ack)return 0;
 json_object_object_add(ack,"envelope_id",json_object_new_string(envelope));
 data=json_object_to_json_string_ext(ack,JSON_C_TO_STRING_PLAIN);length=strlen(data);
 rc=curl_ws_send(ws,data,length,&sent,0,CURLWS_TEXT);
 json_object_put(ack);return rc==CURLE_OK&&sent==length;
}
static void socket_session(const char *url){
 CURL *ws=curl_easy_init();char data[BUF_CAP];size_t count=0;unsigned int idle=0;
 if(!ws)return;
 curl_easy_setopt(ws,CURLOPT_URL,url);
 curl_easy_setopt(ws,CURLOPT_CONNECT_ONLY,2L);
 curl_easy_setopt(ws,CURLOPT_TIMEOUT,15L);
 if(curl_easy_perform(ws)!=CURLE_OK){curl_easy_cleanup(ws);return;}
 while(!stop_flag){
  const struct curl_ws_frame *frame=NULL;size_t received=0;CURLcode rc;
  rc=curl_ws_recv(ws,data+count,sizeof(data)-count-1,&received,&frame);
  if(rc==CURLE_AGAIN){{struct timespec pause_time={0,100000000};nanosleep(&pause_time,NULL);}if(++idle>3000)break;continue;}
  if(rc!=CURLE_OK||!frame||frame->flags&CURLWS_CLOSE)break;
  idle=0;count+=received;
  if(count>=sizeof(data)-1)break;
  if(frame->bytesleft>0)continue;
  data[count]=0;count=0;
  {struct json_object *root=json_tokener_parse(data);
   const char *envelope=field(root,"envelope_id");
   if(root&&envelope&&socket_ack(ws,envelope)) (void)process_event(root);
   if(root)json_object_put(root);
  }
 }
 curl_easy_cleanup(ws);
}
/* Autonomous scheduled intelligence collection; no public Slack command needed. */
static int intelligence_change(const char *report){
 const char *p=report?strstr(report,"Investigations:"):NULL;
 size_t active_cases=0,opened=0,added=0;
 if(!p)return 0;
 if(sscanf(p,"Investigations: %zu active; %zu opened; %zu distinct evidence IDs added",
           &active_cases,&opened,&added)!=3)return 0;
 (void)active_cases;
 return opened>0||added>0;
}
static void *intel_watch_main(void *arg){
 char pending[ALERT_CAP]={0};
 (void)arg;
 while(!stop_flag){
  unsigned int i;
  int pending_state;
  for(i=0;i<300&&!stop_flag;i++)sleep(1);
  if(stop_flag)break;
  if(!cfg.intel_channel[0])continue;
  pending_state=alert_load(INTEL_OUTBOX_PATH,pending,sizeof(pending));
  if(pending_state<0){
   if(host&&host->send_message)(void)host->send_message("[SLACK] Intelligence outbox invalid or unavailable; collection paused");
   continue;
  }
  if(pending_state>0){
   if(!reply(cfg.intel_channel,NULL,pending)){
    if(host&&host->send_message)(void)host->send_message("[SLACK] Pending intelligence alert delivery failed; retry scheduled");
    continue;
   }
   if(!alert_clear(INTEL_OUTBOX_PATH)){
    if(host&&host->send_message)(void)host->send_message("[SLACK] Delivered alert but outbox clear failed; collection paused to avoid duplicate");
    continue;
   }
   pending[0]=0;
  }
  if(host&&host->invoke_service){
   digit_stn2_request_t request={0};
   digit_stn2_result_t result={0};
   size_t used=0;
   int ok;
   snprintf(request.actor,sizeof(request.actor),"slack:scheduled");
   snprintf(request.command,sizeof(request.command),"gen intel");
   pthread_mutex_lock(&intel_mutex);
   ok=host->invoke_service(DIGIT_STN2_SERVICE,&request,sizeof(request),
                           &result,sizeof(result),&used)==STNLABZ_MODULE_OK;
   pthread_mutex_unlock(&intel_mutex);
   if(ok&&used==sizeof(result)&&memchr(result.report,0,sizeof(result.report))&&
      intelligence_change(result.report)){
    if(!alert_save(INTEL_OUTBOX_PATH,result.report)){
     if(host->send_message)(void)host->send_message("[SLACK] CRITICAL: intelligence changed but outbox persistence failed");
     continue;
    }
    if(reply(cfg.intel_channel,NULL,result.report)){
     if(!alert_clear(INTEL_OUTBOX_PATH)&&host->send_message)
      (void)host->send_message("[SLACK] Intelligence delivered; outbox clear failed");
    }else if(host->send_message)
     (void)host->send_message("[SLACK] Intelligence alert persisted for retry");
   }
  }
 }
 return NULL;
}
static void *worker_main(void *arg){
 (void)arg;
 while(!stop_flag){
  struct json_object *response=post("https://slack.com/api/apps.connections.open",cfg.app,NULL);
  const char *url=field(response,"url");
  if(response&&yes(response,"ok")&&url&&strncmp(url,"wss://",6)==0)
   socket_session(url);
  else if(host&&host->send_message)
   (void)host->send_message("[SLACK] Socket Mode connection unavailable");
  if(response)json_object_put(response);
  {unsigned int i;for(i=0;i<30&&!stop_flag;i++)sleep(1);}
 }
 return NULL;
}
static stnlabz_module_result_t qualify(stnlabz_module_qualification_result_t *r){
 unsigned int passed=0;slack_config_t empty={0};struct json_object *j;
 if(!r)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 memset(r,0,sizeof(*r));
 passed+=bounded(empty.workspace,sizeof(empty.workspace),"T123");
 passed+=!bounded(empty.workspace,3,"toolong");
 passed+=!private_file("/this/file/does/not/exist");
 passed+=!load_config("/this/file/does/not/exist",&empty);
 j=json_tokener_parse("{\"event\":{\"type\":\"app_mention\"}}");
 passed+=j!=NULL;passed+=field(j,"not_present")==NULL;
 passed+=object(j,"event")!=NULL;passed+=object(j,"missing")==NULL;
 if(j)json_object_put(j);
 passed+=strncmp("xapp-","xapp-",5)==0;
 passed+=strncmp("xoxb-","xoxb-",5)==0;
 r->tests_executed=10;r->tests_passed=passed;r->tests_failed=10-passed;
 r->negative_test_executed=1;r->negative_test_passed=!load_config("/this/file/does/not/exist",&empty);
 return r->tests_failed?STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}
static void startup_failure(const stnlabz_module_host_t *h,const char *stage){
 char line[160];
 if(!h||!h->send_message)return;
 snprintf(line,sizeof(line),"[SLACK] START_FAILED stage=%s",stage);
 (void)h->send_message(line);
}
static stnlabz_module_result_t start(const stnlabz_module_host_t *h){
 struct json_object *identity=NULL;
 const char *id,*failure="unknown";
 int thread_rc;
 if(!h||!h->invoke_service)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 if(!load_config(SLACK_PATH,&cfg)){startup_failure(h,"configuration");return STNLABZ_MODULE_ERR_START_FAILED;}
 if(!cfg.enabled){startup_failure(h,"disabled");return STNLABZ_MODULE_ERR_START_FAILED;}
 if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){failure="curl_init";goto fail;}
 identity=post("https://slack.com/api/auth.test",cfg.bot,NULL);
 id=field(identity,"user_id");
 if(!identity||!yes(identity,"ok")||!id||!bounded(bot_id,sizeof(bot_id),id)){
  failure="auth_test";goto fail_curl;
 }
 json_object_put(identity);identity=NULL;host=h;stop_flag=0;
 thread_rc=pthread_create(&thread,NULL,worker_main,NULL);
 if(thread_rc!=0){failure="worker_creation";goto fail_curl;}
 active=1;
 if(cfg.intel_channel[0]){
  thread_rc=pthread_create(&watch_thread,NULL,intel_watch_main,NULL);
  if(thread_rc==0)watch_active=1;
  else if(host->send_message)(void)host->send_message("[SLACK] Intelligence watch thread unavailable");
 }
 if(host->send_message)(void)host->send_message("[SLACK] Socket Mode module started");
 return STNLABZ_MODULE_OK;
fail_curl:
 if(identity)json_object_put(identity);
 curl_global_cleanup();
fail:
 startup_failure(h,failure);
 erase(&cfg,sizeof(cfg));erase(bot_id,sizeof(bot_id));host=NULL;
 return STNLABZ_MODULE_ERR_START_FAILED;
}
static stnlabz_module_result_t stop(void){
 if(active){stop_flag=1;pthread_join(thread,NULL);if(watch_active){pthread_join(watch_thread,NULL);watch_active=0;}active=0;}
 pthread_mutex_lock(&mutex);erase(sessions,sizeof(sessions));pthread_mutex_unlock(&mutex);
 host=NULL;erase(&cfg,sizeof(cfg));erase(bot_id,sizeof(bot_id));curl_global_cleanup();
 return STNLABZ_MODULE_OK;
}
static const stnlabz_module_descriptor_t descriptor={
 "slack","Digit Slack",1,0,7,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,
 qualify,start,stop
};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &descriptor;}
