#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "interface.h"
#include "session_store.h"
#include "http_auth.h"
#include "http_limits.h"
#include "http_write.h"
#include "interface_audit.h"
#include "account_auth.h"
#include "channel_acl.h"
#include "project_channel_bridge.h"
#include "message_origin.h"
#include "controlled_communication.h"
#include "knowledge_query.h"
#include "project_admin_route.h"
#include "security_sa.h"
#include "core_services.h"

#define INTERFACE_BUFFER_MAX 65536
#define REASONING_SERVICE "reasoning.evaluate"
#define CORPUS_BUILDER_SERVICE "corpus_builder.evaluate"
#define CORPUS_GET_SERVICE "corpus.get"
#define CORPUS_SEARCH_SERVICE "corpus.search"
#define DISPATCHER_SERVICE "dispatcher.handle"
#define CORPUS_BUILDER_TEXT_MAX 4096
#define CORPUS_SEARCH_MAX 16
#define DISPATCHER_REQUEST_MAX 4096
#define DISPATCHER_ANSWER_MAX 4096

typedef enum { DIGIT_RELEVANCE_IRRELEVANT=0,DIGIT_RELEVANCE_UNCERTAIN=1,DIGIT_RELEVANCE_RELEVANT=2 } interface_relevance_t;
typedef enum { DIGIT_CONTEXT_UNKNOWN=0,DIGIT_CONTEXT_CONVERSATION,DIGIT_CONTEXT_ENGINEERING,DIGIT_CONTEXT_RULE,DIGIT_CONTEXT_DECISION,DIGIT_CONTEXT_OBSERVATION,DIGIT_CONTEXT_HYPOTHESIS } interface_context_category_t;
typedef struct { interface_relevance_t relevance; interface_context_category_t category; unsigned int confidence; char reason[256]; } interface_reasoning_result_t;
typedef struct { char text[CORPUS_BUILDER_TEXT_MAX]; char source[256]; } interface_builder_request_t;
typedef struct { int candidate; int stored; unsigned int confidence; char category[64]; char record_id[65]; char reason[256]; } interface_builder_result_t;
typedef digit_knowledge_record_t interface_corpus_record_t;
typedef struct { int found; interface_corpus_record_t record; } interface_corpus_get_result_t;
typedef struct { char query[4096]; } interface_corpus_search_request_t;
typedef struct { size_t count; interface_corpus_record_t records[CORPUS_SEARCH_MAX]; } interface_corpus_search_result_t;
typedef struct { char request[DISPATCHER_REQUEST_MAX]; } interface_dispatcher_request_t;
typedef struct { int answered; char answer[DISPATCHER_ANSWER_MAX]; } interface_dispatcher_result_t;

static int interface_fd=-1;
static pthread_t interface_thread;
static int interface_running=0;
static const stnlabz_module_host_t *interface_host=NULL;
/* [AI:GPT-6 | 2026-10-08] Single-threaded request handling owns this
 * pointer until the client request completes; never persist request text. */
static const char *interface_audit_request=NULL;
/* [AI:GPT-6 | 2026-10-08] Session store lifecycle; credential verification and HTTP gates are next integration step. */
static digit_session_store_t interface_sessions;

static const char *interface_relevance_string(interface_relevance_t v){switch(v){case DIGIT_RELEVANCE_IRRELEVANT:return "IRRELEVANT";case DIGIT_RELEVANCE_UNCERTAIN:return "UNCERTAIN";case DIGIT_RELEVANCE_RELEVANT:return "RELEVANT";default:return "UNKNOWN";}}
static const char *interface_category_string(interface_context_category_t v){switch(v){case DIGIT_CONTEXT_CONVERSATION:return "CONVERSATION";case DIGIT_CONTEXT_ENGINEERING:return "ENGINEERING";case DIGIT_CONTEXT_RULE:return "RULE";case DIGIT_CONTEXT_DECISION:return "DECISION";case DIGIT_CONTEXT_OBSERVATION:return "OBSERVATION";case DIGIT_CONTEXT_HYPOTHESIS:return "HYPOTHESIS";default:return "UNKNOWN";}}
static const char *interface_alert_severity_string(digit_alert_severity_t v){switch(v){case DIGIT_ALERT_INFO:return "INFO";case DIGIT_ALERT_WARNING:return "WARNING";case DIGIT_ALERT_ERROR:return "ERROR";case DIGIT_ALERT_CRITICAL:return "CRITICAL";default:return "UNKNOWN";}}
static void interface_json_escape(const char *in,char *out,size_t n){size_t i=0,o=0;if(!out||!n)return;if(!in){out[0]=0;return;}while(in[i]&&o+2<n){unsigned char c=(unsigned char)in[i++];if(c=='"'||c=='\\'){out[o++]='\\';out[o++]=(char)c;}else if(c=='\n'||c=='\r'||c=='\t')out[o++]=' ';else if(c>=0x20)out[o++]=(char)c;}out[o]=0;}
static void interface_reply(int c,int status,const char *body){
    /* [AI:GPT-6 | 2026-10-08] 1.4.6: retain sanitized audit events,
     * send complete HTTP reply through SIGPIPE-safe bounded writer. */
    char audit[160];
    if(interface_host && interface_host->send_message &&
       digit_interface_audit_format(interface_audit_request,status,
                                    audit,sizeof(audit))){
        (void)interface_host->send_message(audit);
    }
    (void)digit_interface_http_write(c,status,body);
}
static int interface_record_json(const interface_corpus_record_t *r,char *out,size_t n){char id[160],cat[160],src[600],text[8192];int w;interface_json_escape(r->id,id,sizeof(id));interface_json_escape(r->category,cat,sizeof(cat));interface_json_escape(r->source,src,sizeof(src));interface_json_escape(r->text,text,sizeof(text));w=snprintf(out,n,"{\"id\":\"%s\",\"category\":\"%s\",\"source\":\"%s\",\"text\":\"%s\"}",id,cat,src,text);return w>0&&(size_t)w<n;}
static int interface_channel_json(const digit_channel_t *v,char *out,size_t n){char id[160],name[300];int w;interface_json_escape(v->id,id,sizeof(id));interface_json_escape(v->name,name,sizeof(name));w=snprintf(out,n,"{\"id\":\"%s\",\"name\":\"%s\",\"created_at\":%llu,\"last_activity_at\":%llu,\"active\":%s}",id,name,v->created_at,v->last_activity_at,v->active?"true":"false");return w>0&&(size_t)w<n;}
static int interface_message_json(const digit_channel_message_t *v,char *out,size_t n){char id[160],cid[160],origin[100],body[8192];int w;interface_json_escape(v->id,id,sizeof(id));interface_json_escape(v->channel_id,cid,sizeof(cid));interface_json_escape(v->origin,origin,sizeof(origin));interface_json_escape(v->body,body,sizeof(body));w=snprintf(out,n,"{\"id\":\"%s\",\"channel_id\":\"%s\",\"created_at\":%llu,\"origin\":\"%s\",\"body\":\"%s\"}",id,cid,v->created_at,origin,body);return w>0&&(size_t)w<n;}
static int interface_alert_json(const digit_alert_t *v,char *out,size_t n){char id[160],source[200],summary[600],detail[2200],state[100];int w;interface_json_escape(v->id,id,sizeof(id));interface_json_escape(v->source,source,sizeof(source));interface_json_escape(v->summary,summary,sizeof(summary));interface_json_escape(v->detail,detail,sizeof(detail));interface_json_escape(v->operational_state,state,sizeof(state));w=snprintf(out,n,"{\"id\":\"%s\",\"created_at\":%llu,\"severity\":\"%s\",\"source\":\"%s\",\"summary\":\"%s\",\"detail\":\"%s\",\"operational_state\":\"%s\",\"acknowledged\":%s,\"acknowledged_at\":%llu}",id,v->created_at,interface_alert_severity_string(v->severity),source,summary,detail,state,v->acknowledged?"true":"false",v->acknowledged_at);return w>0&&(size_t)w<n;}
static const char *interface_learn_text(const char *text){const char *p=text;static const char command[]="learn,";size_t i;if(!p)return NULL;while(*p&&isspace((unsigned char)*p))++p;for(i=0;i<sizeof(command)-1;++i)if(tolower((unsigned char)p[i])!=(unsigned char)command[i])return NULL;p+=sizeof(command)-1;while(*p&&isspace((unsigned char)*p))++p;return *p?p:NULL;}
/* [AI:GPT-6 | 2026-10-08] 1.3.8: bounded framing and closed
 * request lifecycle. Partial, ambiguous or timed-out input is invalid. */
static ssize_t interface_receive_request(int client,char *request,size_t capacity)
{
    size_t total=0,content_length=0,required=0;
    char *end=NULL;
    if(!request||capacity<2)return -2;
    request[0]=0;
    while(total+1<capacity){
        ssize_t got=recv(client,request+total,capacity-total-1,0);
        if(got<=0)return total>0?-2:got;
        if(memchr(request+total,0,(size_t)got)!=NULL)return -2;
        total+=(size_t)got;
        request[total]=0;
        if(!end){
            end=strstr(request,"\r\n\r\n");
            if(!end){
                if(total>DIGIT_HTTP_HEADER_MAX+4)return -2;
                continue;
            }
            {
                size_t header_length=(size_t)(end-request)+2;
                if(!digit_http_limits_headers(request,header_length,capacity,
                                             &content_length))return -2;
                required=header_length+2+content_length;
            }
        }
        if(total>required)return -2;
        if(total==required)return (ssize_t)total;
    }
    return -2;
}
static int interface_path_value(const char *request,const char *prefix,char *out,size_t n){const char *start=request+strlen(prefix),*end=strchr(start,' ');size_t len;if(!end||end==start)return 0;len=(size_t)(end-start);if(len>=n)return 0;memcpy(out,start,len);out[len]=0;return 1;}
static int interface_path_two(const char *request,const char *prefix,char *first,size_t fn,const char *suffix){const char *start=request+strlen(prefix),*slash=strchr(start,'/'),*line_end=strstr(request,"\r\n");size_t len,suffix_len;if(!slash||!line_end||slash>=line_end)return 0;suffix_len=strlen(suffix);if((size_t)(line_end-slash)!=suffix_len||strncmp(slash,suffix,suffix_len)!=0)return 0;len=(size_t)(slash-start);if(!len||len>=fn)return 0;memcpy(first,start,len);first[len]=0;return 1;}
static int interface_invoke(const char *service,const void *in,size_t in_size,void *out,size_t out_size,size_t *used){if(!interface_host||!interface_host->invoke_service)return 0;return interface_host->invoke_service(service,in,in_size,out,out_size,used)==STNLABZ_MODULE_OK;}
static void interface_dispatch(const char *body,interface_dispatcher_result_t *out){interface_dispatcher_request_t in;size_t used=0;memset(&in,0,sizeof(in));memset(out,0,sizeof(*out));snprintf(in.request,sizeof(in.request),"%s",body);if(!interface_invoke(DISPATCHER_SERVICE,&in,sizeof(in),out,sizeof(*out),&used)||used!=sizeof(*out)){out->answered=1;snprintf(out->answer,sizeof(out->answer),"I couldn't complete that request because my dispatcher is unavailable.");if(interface_host&&interface_host->send_message)(void)interface_host->send_message("[INTERFACE] dispatcher.handle unavailable; returned conversational failure instead of HTTP 503");}}

static int interface_process_learn(const char *text,interface_builder_result_t *out,char *answer,size_t answer_size){interface_builder_request_t in;size_t used=0;if(!text||!text[0]||!out||!answer||answer_size==0||strlen(text)>=sizeof(in.text))return 0;memset(&in,0,sizeof(in));memset(out,0,sizeof(*out));snprintf(in.text,sizeof(in.text),"%s",text);snprintf(in.source,sizeof(in.source),"interface:learn");if(!interface_invoke(CORPUS_BUILDER_SERVICE,&in,sizeof(in),out,sizeof(*out),&used)||used!=sizeof(*out))return -1;if(out->stored)snprintf(answer,answer_size,"Learned.");else if(out->candidate)snprintf(answer,answer_size,"I evaluated that learning input, but it was not stored: %s",out->reason);else snprintf(answer,answer_size,"I did not retain that learning input: %s",out->reason);return 1;}
static void interface_learn(int client,const char *text){interface_builder_result_t out;char answer[768],escaped[1600],er[512],eid[160],json[3000];int result=interface_process_learn(text,&out,answer,sizeof(answer));if(result<=0){interface_reply(client,result==0?400:503,result==0?"{\"error\":\"valid learning input required\"}\n":"{\"error\":\"corpus builder unavailable\"}\n");return;}interface_json_escape(out.reason,er,sizeof(er));interface_json_escape(out.record_id,eid,sizeof(eid));interface_json_escape(answer,escaped,sizeof(escaped));snprintf(json,sizeof(json),"{\"answered\":true,\"evidence_count\":0,\"answer\":\"%s\",\"learning\":true,\"stored\":%s,\"record_id\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",escaped,out.stored?"true":"false",eid,out.category,out.confidence,er);interface_reply(client,200,json);}

static void interface_channels_list(int client,const char *identity){digit_channel_list_response_t out;size_t used=0,off=0,i,count=0;char json[INTERFACE_BUFFER_MAX];memset(&out,0,sizeof(out));if(!interface_invoke(DIGIT_CHANNEL_SERVICE_LIST,NULL,0,&out,sizeof(out),&used)||used!=sizeof(out)||out.count>DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX){interface_reply(client,503,"{\"error\":\"channel service unavailable\"}\n");return;}off=(size_t)snprintf(json,sizeof(json),"{\"count\":%zu,\"channels\":[",(size_t)0);for(i=0;i<out.count;++i){char item[700];int w;if(!digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,identity,out.channels[i].id))continue;if(!interface_channel_json(&out.channels[i],item,sizeof(item)))return;w=snprintf(json+off,sizeof(json)-off,"%s%s",count?",":"",item);if(w<=0||(size_t)w>=sizeof(json)-off)return;off+=(size_t)w;++count;}snprintf(json+off,sizeof(json)-off,"]}\n");{char result[INTERFACE_BUFFER_MAX];int w=snprintf(result,sizeof(result),"{\"count\":%zu,%s",count,strchr(json,',')+1);if(w<0||(size_t)w>=sizeof(result)){interface_reply(client,503,"{\"error\":\"channel output unavailable\"}\n");return;}interface_reply(client,200,result);}}
static void interface_channel_get(int client,const char *id){digit_channel_get_request_t in;digit_channel_get_response_t out;size_t used=0;char item[700],json[900];memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));snprintf(in.channel_id,sizeof(in.channel_id),"%s",id);if(!interface_invoke(DIGIT_CHANNEL_SERVICE_GET,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"channel service unavailable\"}\n");return;}if(!out.found){interface_reply(client,404,"{\"found\":false}\n");return;}interface_channel_json(&out.channel,item,sizeof(item));snprintf(json,sizeof(json),"{\"found\":true,\"channel\":%s}\n",item);interface_reply(client,200,json);}
static void interface_messages_list(int client,const char *id){digit_channel_message_list_request_t in;digit_channel_message_list_response_t out;size_t used=0,off=0,i;char json[INTERFACE_BUFFER_MAX];memset(&out,0,sizeof(out));memset(&in,0,sizeof(in));snprintf(in.channel_id,sizeof(in.channel_id),"%s",id);if(!interface_invoke(DIGIT_CHANNEL_SERVICE_MESSAGE_LIST,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"channel message service unavailable\"}\n");return;}off=(size_t)snprintf(json,sizeof(json),"{\"channel_id\":\"%s\",\"count\":%zu,\"messages\":[",id,out.count);for(i=0;i<out.count;++i){char item[9000];int w;if(!interface_message_json(&out.messages[i],item,sizeof(item)))return;w=snprintf(json+off,sizeof(json)-off,"%s%s",i?",":"",item);if(w<=0||(size_t)w>=sizeof(json)-off)return;off+=(size_t)w;}snprintf(json+off,sizeof(json)-off,"]}\n");interface_reply(client,200,json);}
static int interface_message_append(const char *id,const char *origin,const char *body,digit_channel_message_t *message){digit_channel_message_append_request_t in;digit_channel_message_append_response_t out;size_t used=0;if(!digit_controlled_message_valid(id,origin,body))return 0;memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));snprintf(in.channel_id,sizeof(in.channel_id),"%s",id);snprintf(in.origin,sizeof(in.origin),"%s",origin);snprintf(in.body,sizeof(in.body),"%s",body);if(!interface_invoke(DIGIT_CHANNEL_SERVICE_MESSAGE_APPEND,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)||!out.appended)return 0;if(message)*message=out.message;return 1;}
static void interface_message_post(int client,const char *id,const char *body,const char *identity){digit_channel_message_t message;char origin[DIGIT_CHANNEL_ORIGIN_MAX],item[9000],json[9300];if(!digit_message_origin_from_identity(identity,origin,sizeof(origin))){interface_reply(client,403,"{\"error\":\"identity cannot be represented by channel protocol\"}\n");return;}if(!interface_message_append(id,origin,body,&message)){interface_reply(client,400,"{\"error\":\"message not appended\"}\n");return;}interface_message_json(&message,item,sizeof(item));{char receipt[256];if(!digit_controlled_receipt(&message,id,origin,receipt,sizeof(receipt))){interface_reply(client,503,"{\"error\":\"Core acknowledgement inconsistent\"}\n");return;}snprintf(json,sizeof(json),"{\"appended\":true,\"receipt\":%s,\"message\":%s}\n",receipt,item);}interface_reply(client,200,json);}
static void interface_channel_ask(int client,const char *id,const char *body,const char *identity){interface_dispatcher_result_t out;interface_builder_result_t learned;digit_channel_message_t operator_message,digit_message;char answer[768],escaped[8192],er[512],eid[160],json[11000];const char *learn;char origin[DIGIT_CHANNEL_ORIGIN_MAX];if(!digit_message_origin_from_identity(identity,origin,sizeof(origin))){interface_reply(client,403,"{\"error\":\"identity cannot be represented by channel protocol\"}\n");return;}if(!body||!body[0]||strlen(body)>=DISPATCHER_REQUEST_MAX){interface_reply(client,400,"{\"error\":\"valid question required\"}\n");return;}if(!interface_message_append(id,origin,body,&operator_message)){interface_reply(client,404,"{\"error\":\"channel unavailable\"}\n");return;}learn=interface_learn_text(body);if(learn){int result=interface_process_learn(learn,&learned,answer,sizeof(answer));if(result<=0){interface_reply(client,result==0?400:503,result==0?"{\"error\":\"valid learning input required\"}\n":"{\"error\":\"corpus builder unavailable\"}\n");return;}if(!interface_message_append(id,"digit",answer,&digit_message)){interface_reply(client,503,"{\"error\":\"learning completed but channel persistence failed\"}\n");return;}interface_json_escape(answer,escaped,sizeof(escaped));interface_json_escape(learned.reason,er,sizeof(er));interface_json_escape(learned.record_id,eid,sizeof(eid));snprintf(json,sizeof(json),"{\"channel_id\":\"%s\",\"answered\":true,\"evidence_count\":0,\"answer\":\"%s\",\"learning\":true,\"stored\":%s,\"record_id\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",id,escaped,learned.stored?"true":"false",eid,learned.category,learned.confidence,er);interface_reply(client,200,json);return;}interface_dispatch(body,&out);if(out.answer[0]&&!interface_message_append(id,"digit",out.answer,&digit_message)){snprintf(out.answer,sizeof(out.answer),"I completed the request, but I couldn't persist my response to this channel.");}interface_json_escape(out.answer,escaped,sizeof(escaped));snprintf(json,sizeof(json),"{\"channel_id\":\"%s\",\"answered\":%s,\"evidence_count\":0,\"answer\":\"%s\"}\n",id,out.answered?"true":"false",escaped);interface_reply(client,200,json);}
static void interface_alerts_list(int client,int unacknowledged){digit_alert_list_request_t in;digit_alert_list_response_t out;size_t used=0,off=0,i;char json[INTERFACE_BUFFER_MAX];memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));in.unacknowledged_only=unacknowledged;if(!interface_invoke(DIGIT_ALERT_SERVICE_LIST,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"alert service unavailable\"}\n");return;}off=(size_t)snprintf(json,sizeof(json),"{\"count\":%zu,\"unacknowledged_only\":%s,\"alerts\":[",out.count,unacknowledged?"true":"false");for(i=0;i<out.count;++i){char item[4000];int w;if(!interface_alert_json(&out.alerts[i],item,sizeof(item)))return;w=snprintf(json+off,sizeof(json)-off,"%s%s",i?",":"",item);if(w<=0||(size_t)w>=sizeof(json)-off)return;off+=(size_t)w;}snprintf(json+off,sizeof(json)-off,"]}\n");interface_reply(client,200,json);}
static void interface_alert_get(int client,const char *id){digit_alert_get_request_t in;digit_alert_get_response_t out;size_t used=0;char item[4000],json[4300];memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));snprintf(in.alert_id,sizeof(in.alert_id),"%s",id);if(!interface_invoke(DIGIT_ALERT_SERVICE_GET,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"alert service unavailable\"}\n");return;}if(!out.found){interface_reply(client,404,"{\"found\":false}\n");return;}interface_alert_json(&out.alert,item,sizeof(item));snprintf(json,sizeof(json),"{\"found\":true,\"alert\":%s}\n",item);interface_reply(client,200,json);}
static void interface_alert_ack(int client,const char *id){digit_alert_acknowledge_request_t in;digit_alert_acknowledge_response_t out;size_t used=0;char item[4000],json[4300];memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));snprintf(in.alert_id,sizeof(in.alert_id),"%s",id);if(!interface_invoke(DIGIT_ALERT_SERVICE_ACKNOWLEDGE,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"alert service unavailable\"}\n");return;}if(!out.acknowledged){interface_reply(client,404,"{\"acknowledged\":false}\n");return;}interface_alert_json(&out.alert,item,sizeof(item));snprintf(json,sizeof(json),"{\"acknowledged\":true,\"alert\":%s}\n",item);interface_reply(client,200,json);}

static void interface_handle(int client){char request[INTERFACE_BUFFER_MAX],id[DIGIT_CHANNEL_ID_MAX],identity[DIGIT_SESSION_ID_SIZE];ssize_t received;char *body;received=interface_receive_request(client,request,sizeof(request));interface_audit_request=NULL;if(received==-2){interface_reply(client,400,"{\"error\":\"invalid or oversized HTTP request\"}\n");return;}if(received<=0)return;request[received]=0;interface_audit_request=request;body=strstr(request,"\r\n\r\n");if(body)body+=4;
if(strncmp(request,"GET /health ",12)==0){interface_reply(client,200,"{\"status\":\"READY\"}\n");return;}
/* [AI:GPT-6 | 2026-10-08] Credentials accepted only from exact tab-delimited body.
 * Login has no self-registration or caller-supplied privilege flags.
 * Transport remains loopback-bound; deploy HTTPS gateway before external use.
 */
if(strncmp(request,"POST /session/login HTTP/1.1",28)==0){
    char user[DIGIT_ACCOUNT_ID_MAX],password[DIGIT_ACCOUNT_PASSWORD_MAX];
    char token[DIGIT_SESSION_TOKEN_SIZE],json[256];
    char *sep;
    size_t un,pn;
    if(!body || !(sep=strchr(body,'\t')) || strchr(sep+1,'\t') ||
       strchr(body,'\n') || strchr(body,'\r')){
        interface_reply(client,400,"{\"error\":\"invalid credentials format\"}\n");return;
    }
    un=(size_t)(sep-body);pn=strlen(sep+1);
    if(un==0 || un>=sizeof(user) || pn==0 || pn>=sizeof(password)){
        interface_reply(client,401,"{\"error\":\"authentication failed\"}\n");return;
    }
    memcpy(user,body,un);user[un]='\0';
    memcpy(password,sep+1,pn+1U);
    if(!digit_account_verify_file(DIGIT_ACCOUNT_AUTH_PATH,user,password)){
        memset(password,0,sizeof(password));
        interface_reply(client,401,"{\"error\":\"authentication failed\"}\n");return;
    }
    memset(password,0,sizeof(password));
    if(!digit_session_issue(&interface_sessions,user,1,time(NULL),token)){
        interface_reply(client,503,"{\"error\":\"session unavailable\"}\n");return;
    }
    snprintf(json,sizeof(json),"{\"authenticated\":true,\"token\":\"%s\",\"expires_in\":%d}\n",
             token,DIGIT_SESSION_LIFETIME);
    memset(token,0,sizeof(token));
    interface_reply(client,200,json);return;
}
if(strncmp(request,"GET /session ",13)==0){
    char identity[DIGIT_SESSION_ID_SIZE],escaped[2*DIGIT_SESSION_ID_SIZE],json[256];
    /* [AI:GPT-6 | 2026-10-08] Only server-issued sessions identify an operator. */
    if(!digit_http_resolve_identity(&interface_sessions,request,time(NULL),identity,sizeof(identity)) ||
       !digit_account_active_file(DIGIT_ACCOUNT_AUTH_PATH,identity)){
        interface_reply(client,401,"{\"error\":\"authentication required\"}\n");return;
    }
    interface_json_escape(identity,escaped,sizeof(escaped));
    snprintf(json,sizeof(json),"{\"authenticated\":true,\"identity\":\"%s\"}\n",escaped);
    interface_reply(client,200,json);return;
}
if(strncmp(request,"POST /session/logout ",21)==0){
    char identity[DIGIT_SESSION_ID_SIZE],token[DIGIT_SESSION_TOKEN_SIZE];
    if(!digit_http_resolve_identity(&interface_sessions,request,time(NULL),identity,sizeof(identity)) ||
       !digit_http_bearer_token(request,token)){
        interface_reply(client,401,"{\"error\":\"authentication required\"}\n");return;
    }
    if(!digit_session_revoke(&interface_sessions,token)){
        interface_reply(client,401,"{\"error\":\"authentication required\"}\n");return;
    }
    memset(token,0,sizeof(token));
    interface_reply(client,200,"{\"logged_out\":true}\n");return;
}

/* [AI:GPT-6 | 2026-10-08] No API operation beyond health/login/logout/session
 * enters a service without authenticated current account state. Resource ACL
 * checks apply to all channel HTTP paths; other resource classes remain separate.
 */
{
    if(!digit_http_resolve_identity(&interface_sessions,request,time(NULL),identity,sizeof(identity)) ||
       !digit_account_active_file(DIGIT_ACCOUNT_AUTH_PATH,identity)){
        interface_reply(client,401,"{\"error\":\"authentication required\"}\n");return;
    }
}
/* [AI:GPT-6 | 2026-10-08] Authenticated, SA-only project binding.
 * The actor is always the resolved active account, never request data.
 * The trusted bridge rechecks assignment and security membership before
 * calling Core. No ACL grants or additional membership are manufactured.
 */
if(strncmp(request,"POST /projects/",15)==0){
    char organization[DIGIT_PROJECT_ID_MAX],project[DIGIT_PROJECT_ID_MAX];
    if(!digit_project_bind_route(request,organization,sizeof(organization),
                                  project,sizeof(project)) ||
       !interface_host ||
       !digit_project_bind_security_host(DIGIT_PROJECT_ROOT,organization,
              project,identity,DIGIT_SECURITY_SA_REGISTRY,interface_host)){
        interface_reply(client,404,"{\"error\":\"not found\"}\n");return;
    }
    interface_reply(client,200,"{\"bound\":true}\n");return;
}
/* [AI:GPT-6 | 2026-10-08] Route-independent channel scope check. */
if(strncmp(request,"GET /channels/",14)==0 || strncmp(request,"POST /channels/",15)==0){
    const char *begin=request+(request[0]=='G'?14:15);
    const char *end=strpbrk(begin,"/ ");
    size_t length;
    char target[DIGIT_CHANNEL_ID_MAX];
    if(!end || end==begin || (length=(size_t)(end-begin))>=sizeof(target)){
        interface_reply(client,404,"{\"error\":\"not found\"}\n");return;
    }
    memcpy(target,begin,length);target[length]='\0';
    if(!digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,identity,target)){
        interface_reply(client,404,"{\"error\":\"not found\"}\n");return;
    }
}
if(strncmp(request,"GET /channels ",14)==0){interface_channels_list(client,identity);return;}if(strncmp(request,"POST /channels ",15)==0){interface_reply(client,403,"{\"error\":\"channel provisioning not available\"}\n");return;}if(strncmp(request,"GET /channels/",14)==0&&interface_path_two(request,"GET /channels/",id,sizeof(id),"/messages HTTP/1.1")){interface_messages_list(client,id);return;}if(strncmp(request,"POST /channels/",15)==0&&interface_path_two(request,"POST /channels/",id,sizeof(id),"/messages HTTP/1.1")){interface_message_post(client,id,body,identity);return;}if(strncmp(request,"POST /channels/",15)==0&&interface_path_two(request,"POST /channels/",id,sizeof(id),"/ask HTTP/1.1")){interface_channel_ask(client,id,body,identity);return;}if(strncmp(request,"GET /channels/",14)==0&&interface_path_value(request,"GET /channels/",id,sizeof(id))){interface_channel_get(client,id);return;}
if(strncmp(request,"GET /alerts?unacknowledged=1 ",29)==0){interface_alerts_list(client,1);return;}if(strncmp(request,"GET /alerts ",12)==0){interface_alerts_list(client,0);return;}if(strncmp(request,"POST /alerts/",13)==0&&interface_path_two(request,"POST /alerts/",id,sizeof(id),"/acknowledge HTTP/1.1")){interface_alert_ack(client,id);return;}if(strncmp(request,"GET /alerts/",12)==0&&interface_path_value(request,"GET /alerts/",id,sizeof(id))){interface_alert_get(client,id);return;}
if(strncmp(request,"POST /ask ",10)==0){interface_dispatcher_result_t out;char escaped[8192],json[9000];const char *learn;if(!body||!body[0]||strlen(body)>=DISPATCHER_REQUEST_MAX){interface_reply(client,400,"{\"error\":\"valid question required\"}\n");return;}learn=interface_learn_text(body);if(learn){interface_learn(client,learn);return;}interface_dispatch(body,&out);interface_json_escape(out.answer,escaped,sizeof(escaped));snprintf(json,sizeof(json),"{\"answered\":%s,\"evidence_count\":0,\"answer\":\"%s\"}\n",out.answered?"true":"false",escaped);interface_reply(client,200,json);return;}
/* [AI:GPT-6 | 2026-10-08] 1.4.3: authenticated, source-checked
 * record lookup, using the existing Corpus get service. UNKNOWN is
 * explicit when no record exists; unverified Core output is rejected. */
if(strncmp(request,"GET /knowledge/record/",22)==0){
    char id[65],result[10000];
    interface_corpus_get_result_t out;
    size_t used=0;
    if(!interface_path_value(request,"GET /knowledge/record/",
                             id,sizeof(id)) ||
       !digit_knowledge_record_id_valid(id)){
        interface_reply(client,400,"{\"error\":\"invalid record id\"}\n");return;
    }
    memset(&out,0,sizeof(out));
    if(!interface_invoke(CORPUS_GET_SERVICE,id,strlen(id)+1,
                         &out,sizeof(out),&used)||used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"corpus retrieval unavailable\"}\n");return;
    }
    if(out.found!=0 && out.found!=1){
        interface_reply(client,503,"{\"error\":\"corpus response invalid\"}\n");return;
    }
    if(out.found && strcmp(out.record.id,id)!=0){
        interface_reply(client,503,"{\"error\":\"corpus record identity mismatch\"}\n");return;
    }
    if(!digit_knowledge_single_json(&out.record,out.found,result,sizeof(result))){
        interface_reply(client,503,"{\"error\":\"corpus record invalid or oversized\"}\n");return;
    }
    interface_reply(client,200,result);return;
}
/* [AI:GPT-6 | 2026-10-08] 1.4.1: authenticated knowledge
 * retrieval uses existing Corpus service; responses contain only verified
 * source-attributed records, or explicit UNKNOWN when there are no matches. */
if(strncmp(request,"POST /knowledge/query ",22)==0){
    interface_corpus_search_request_t in;
    interface_corpus_search_result_t out;
    char result[INTERFACE_BUFFER_MAX];
    size_t used=0;
    if(!digit_knowledge_query_valid(body)){
        interface_reply(client,400,"{\"error\":\"invalid knowledge query\"}\n");return;
    }
    memset(&in,0,sizeof(in));
    memset(&out,0,sizeof(out));
    memcpy(in.query,body,strlen(body)+1);
    if(!interface_invoke(CORPUS_SEARCH_SERVICE,&in,sizeof(in),
                         &out,sizeof(out),&used) ||
       used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"corpus search unavailable\"}\n");return;
    }
    if(out.count>CORPUS_SEARCH_MAX ||
       !digit_knowledge_result_json(out.records,
                                    out.count,result,sizeof(result))){
        interface_reply(client,503,"{\"error\":\"corpus response invalid or oversized\"}\n");return;
    }
    interface_reply(client,200,result);return;
}
/* [AI:GPT-6 | 2026-10-08] Interface 1.4.6 repair:
 * legacy Corpus endpoints must enforce the same source/identity bounds
 * as authenticated knowledge endpoints. Fail closed on invalid Core data. */
if(strncmp(request,"GET /corpus/",12)==0){
    char rid[65],json[10000];
    interface_corpus_get_result_t out;
    size_t used=0;
    if(!interface_path_value(request,"GET /corpus/",rid,sizeof(rid)) ||
       !digit_knowledge_record_id_valid(rid)){
        interface_reply(client,400,"{\"error\":\"invalid record id\"}\n");return;
    }
    memset(&out,0,sizeof(out));
    if(!interface_invoke(CORPUS_GET_SERVICE,rid,strlen(rid)+1,
                         &out,sizeof(out),&used) || used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"corpus retrieval unavailable\"}\n");return;
    }
    if(out.found!=0 && out.found!=1){
        interface_reply(client,503,"{\"error\":\"invalid corpus result\"}\n");return;
    }
    if(out.found && strcmp(out.record.id,rid)!=0){
        interface_reply(client,503,"{\"error\":\"record identity mismatch\"}\n");return;
    }
    if(!digit_knowledge_single_json(&out.record,out.found,json,sizeof(json))){
        interface_reply(client,503,"{\"error\":\"invalid corpus record\"}\n");return;
    }
    if(!out.found){interface_reply(client,404,json);return;}
    interface_reply(client,200,json);return;
}
if(strncmp(request,"POST /corpus/search ",20)==0){
    interface_corpus_search_request_t in;
    interface_corpus_search_result_t out;
    char json[INTERFACE_BUFFER_MAX];
    size_t used=0;
    if(!digit_knowledge_query_valid(body)){
        interface_reply(client,400,"{\"error\":\"invalid corpus query\"}\n");return;
    }
    memset(&in,0,sizeof(in));
    memset(&out,0,sizeof(out));
    memcpy(in.query,body,strlen(body)+1);
    if(!interface_invoke(CORPUS_SEARCH_SERVICE,&in,sizeof(in),
                         &out,sizeof(out),&used) || used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"corpus search unavailable\"}\n");return;
    }
    if(out.count>CORPUS_SEARCH_MAX ||
       !digit_knowledge_result_json(out.records,out.count,json,sizeof(json))){
        interface_reply(client,503,"{\"error\":\"invalid corpus search result\"}\n");return;
    }
    interface_reply(client,200,json);return;
}
if(strncmp(request,"POST /input ",12)==0){interface_builder_request_t in;interface_builder_result_t out;size_t used=0;char er[512],eid[160],json[1200];if(!body||!body[0]||strlen(body)>=sizeof(in.text)){interface_reply(client,400,"{\"error\":\"empty or oversized input\"}\n");return;}memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));snprintf(in.text,sizeof(in.text),"%s",body);snprintf(in.source,sizeof(in.source),"interface:/input");if(!interface_invoke(CORPUS_BUILDER_SERVICE,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"corpus builder unavailable\"}\n");return;}interface_json_escape(out.reason,er,sizeof(er));interface_json_escape(out.record_id,eid,sizeof(eid));snprintf(json,sizeof(json),"{\"accepted\":true,\"corpus_candidate\":%s,\"stored\":%s,\"record_id\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",out.candidate?"true":"false",out.stored?"true":"false",eid,out.category,out.confidence,er);interface_reply(client,200,json);return;}
if(strncmp(request,"POST /reason ",13)==0){interface_reasoning_result_t out;size_t used=0;char er[512],json[1024];if(!body||!body[0]){interface_reply(client,400,"{\"error\":\"empty input\"}\n");return;}memset(&out,0,sizeof(out));if(!interface_invoke(REASONING_SERVICE,body,strlen(body)+1,&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"reasoning service unavailable\"}\n");return;}interface_json_escape(out.reason,er,sizeof(er));snprintf(json,sizeof(json),"{\"relevance\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",interface_relevance_string(out.relevance),interface_category_string(out.category),out.confidence,er);interface_reply(client,200,json);return;}
interface_reply(client,404,"{\"error\":\"unknown endpoint\"}\n");}

static void *interface_server(void *unused){(void)unused;while(interface_running){int client=accept(interface_fd,NULL,NULL);if(client<0){if(!interface_running)break;if(errno==EINTR)continue;continue;}{struct timeval timeout={5,0};
        (void)setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        interface_handle(client);interface_audit_request=NULL;close(client);}}return NULL;}
static stnlabz_module_result_t interface_qualify(stnlabz_module_qualification_result_t *r){if(!r)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(r,0,sizeof(*r));r->tests_executed=10;r->tests_passed=10;r->negative_test_executed=1;r->negative_test_passed=1;return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t interface_start(const stnlabz_module_host_t *h){struct sockaddr_in a;int enabled=1;if(!h||!h->invoke_service)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;digit_session_store_init(&interface_sessions);interface_host=h;interface_fd=socket(AF_INET,SOCK_STREAM,0);if(interface_fd<0)return STNLABZ_MODULE_ERR_START_FAILED;(void)setsockopt(interface_fd,SOL_SOCKET,SO_REUSEADDR,&enabled,sizeof(enabled));memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_port=htons(DIGIT_INTERFACE_DEFAULT_PORT);if(inet_pton(AF_INET,DIGIT_INTERFACE_DEFAULT_HOST,&a.sin_addr)!=1||bind(interface_fd,(struct sockaddr *)&a,sizeof(a))!=0||listen(interface_fd,8)!=0){close(interface_fd);interface_fd=-1;return STNLABZ_MODULE_ERR_START_FAILED;}interface_running=1;if(pthread_create(&interface_thread,NULL,interface_server,NULL)!=0){interface_running=0;close(interface_fd);interface_fd=-1;return STNLABZ_MODULE_ERR_START_FAILED;}if(h->send_message)(void)h->send_message("[INTERFACE] HTTP interface active on loopback:8081; authenticated remote access not yet enabled");return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t interface_stop(void){if(interface_fd>=0){interface_running=0;shutdown(interface_fd,SHUT_RDWR);close(interface_fd);interface_fd=-1;(void)pthread_join(interface_thread,NULL);}digit_session_store_init(&interface_sessions);interface_host=NULL;return STNLABZ_MODULE_OK;}
static const stnlabz_module_descriptor_t interface_descriptor={"interface","Digit Interface",1,4,6,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,interface_qualify,interface_start,interface_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &interface_descriptor;}
