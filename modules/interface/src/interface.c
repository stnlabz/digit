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
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
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
#include "message_results.h"
#include "channel_results.h"
#include "alert_results.h"
#include "knowledge_query.h"
#include "corpus_response.h"
#include "builder_response.h"
#include "dashboard_response.h"
#include "channel_create_policy.h"
#include "project_admin_route.h"
#include "project_list.h"
#include "grant_request.h"
#include "grant_store.h"
#include "alerts_channel.h"
#include "security_sa.h"
#include "sa_management.h"
#include "core_services.h"

#define INTERFACE_BUFFER_MAX 65536
/* [AI:GPT-6 | 2026-10-08] Remote operation requires server TLS identity. */
#define DIGIT_INTERFACE_TLS_CERT "/opt/digit/state/auth/interface-cert.pem"
#define DIGIT_INTERFACE_TLS_KEY "/opt/digit/state/auth/interface-key.pem"
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
typedef digit_interface_builder_result_t interface_builder_result_t;
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
static SSL_CTX *interface_tls_context=NULL;
static SSL *interface_tls_client=NULL;
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
    if(interface_tls_client){
        char header[512];
        const char *reason=status==200?"OK":status==400?"Bad Request":
                           status==401?"Unauthorized":status==403?"Forbidden":
                           status==404?"Not Found":status==503?"Service Unavailable":"Bad Request";
        int n=snprintf(header,sizeof(header),
          "HTTP/1.1 %d %s\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
          status,reason,strlen(body));
        size_t off;
        if(n<=0||(size_t)n>=sizeof(header))return;
        for(off=0;off<(size_t)n;){int written=SSL_write(interface_tls_client,header+off,(int)((size_t)n-off));if(written<=0)return;off+=(size_t)written;}
        for(off=0;off<strlen(body);){int written=SSL_write(interface_tls_client,body+off,(int)(strlen(body)-off));if(written<=0)return;off+=(size_t)written;}
    }else (void)digit_interface_http_write(c,status,body);
}
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
        ssize_t got=interface_tls_client ? SSL_read(interface_tls_client,request+total,(int)(capacity-total-1)) : recv(client,request+total,capacity-total-1,0);
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

static int interface_process_learn(const char *text,interface_builder_result_t *out,char *answer,size_t answer_size){interface_builder_request_t in;size_t used=0;if(!text||!text[0]||!out||!answer||answer_size==0||strlen(text)>=sizeof(in.text))return 0;memset(&in,0,sizeof(in));memset(out,0,sizeof(*out));snprintf(in.text,sizeof(in.text),"%s",text);snprintf(in.source,sizeof(in.source),"interface:learn");if(!interface_invoke(CORPUS_BUILDER_SERVICE,&in,sizeof(in),out,sizeof(*out),&used)||used!=sizeof(*out)||!digit_interface_builder_result_valid(out))return -1;if(out->stored)snprintf(answer,answer_size,"Learned.");else if(out->candidate)snprintf(answer,answer_size,"I evaluated that learning input, but it was not stored: %s",out->reason);else snprintf(answer,answer_size,"I did not retain that learning input: %s",out->reason);return 1;}
static void interface_learn(int client,const char *text){interface_builder_result_t out;char answer[768],escaped[1600],er[512],eid[160],json[3000];int result=interface_process_learn(text,&out,answer,sizeof(answer));if(result<=0){interface_reply(client,result==0?400:503,result==0?"{\"error\":\"valid learning input required\"}\n":"{\"error\":\"corpus builder unavailable\"}\n");return;}interface_json_escape(out.reason,er,sizeof(er));interface_json_escape(out.record_id,eid,sizeof(eid));interface_json_escape(answer,escaped,sizeof(escaped));snprintf(json,sizeof(json),"{\"answered\":true,\"evidence_count\":0,\"answer\":\"%s\",\"learning\":true,\"stored\":%s,\"record_id\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",escaped,out.stored?"true":"false",eid,out.category,out.confidence,er);interface_reply(client,200,json);}

/* [AI:GPT-6 | 2026-10-08] 1.4.10: validate all Core
 * channel records before ACL filtering; avoid partial/truncated JSON. */
static void interface_channels_list(int client,const char *identity)
{
    digit_channel_list_response_t out;
    size_t used=0,off=0,i,visible=0;
    char json[INTERFACE_BUFFER_MAX],items[INTERFACE_BUFFER_MAX];
    int n;
    memset(&out,0,sizeof(out));
    if(!interface_invoke(DIGIT_CHANNEL_SERVICE_LIST,NULL,0,
                         &out,sizeof(out),&used) || used!=sizeof(out) ||
       !digit_interface_channels_valid(out.channels,out.count,
                                       DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX)){
        interface_reply(client,503,"{\"error\":\"invalid channel list\"}\n");
        return;
    }
    items[0]=0;
    for(i=0;i<out.count;++i){
        char item[700];
        if(!digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,
                                         identity,out.channels[i].id))continue;
        if(!interface_channel_json(&out.channels[i],item,sizeof(item)))goto invalid;
        /* [AI:GPT-6 | 2026-10-09] Bind displayed channel scope to
         * protected project records, never to channel-name heuristics. */
        {
            char owner[64]={0},project[64]={0};
            int owner_status=digit_project_security_owner(DIGIT_PROJECT_ROOT,
                out.channels[i].id,owner,sizeof(owner),project,sizeof(project));
            char alert_id[DIGIT_CHANNEL_ID_MAX];
            size_t length;
            if(owner_status<0)goto invalid;
            if(owner_status==0&&
               digit_alerts_channel_lookup(DIGIT_PROJECT_ROOT,"stn-labz",
                   "operations",alert_id,sizeof(alert_id))&&
               strcmp(alert_id,out.channels[i].id)==0){
                strcpy(owner,"stn-labz");strcpy(project,"operations");
            }
            length=strlen(item);
            if(owner[0]){
                int written;
                if(!length||item[length-1]!='}')goto invalid;
                written=snprintf(item+length-1,sizeof(item)-length+1,
                     ",\"organization\":\"%s\",\"project\":\"%s\"}",
                     owner,project);
                if(written<0||(size_t)written>=sizeof(item)-length+1)goto invalid;
            }
        }
        n=snprintf(items+off,sizeof(items)-off,"%s%s",visible?",":"",item);
        if(n<0 || (size_t)n>=sizeof(items)-off)goto invalid;
        off+=(size_t)n;
        ++visible;
    }
    /* [AI:GPT-6 | 2026-10-09] Include only SA-verified organizations,
     * allowing an empty-but-authorized workspace to remain visible. */
    {
        int stn=digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,"stn-labz",identity);
        int chaos=digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,"team-chaos",identity);
        n=snprintf(json,sizeof(json),
           "{\"count\":%zu,\"organizations\":[%s%s%s],\"channels\":[%s]}\n",
           visible,stn?"\"stn-labz\"":"",stn&&chaos?",":"",
           chaos?"\"team-chaos\"":"",items);
    }
    if(n<0 || (size_t)n>=sizeof(json))goto invalid;
    interface_reply(client,200,json);
    return;
invalid:
    interface_reply(client,503,"{\"error\":\"channel serialization failed\"}\n");
}
static void interface_channel_get(int client,const char *id)
{
    digit_channel_get_request_t in;
    digit_channel_get_response_t out;
    size_t used=0;
    char item[700],json[900];
    int n;
    if(!digit_interface_channel_exact_valid(NULL,0,id)){
        interface_reply(client,400,"{\"error\":\"invalid channel id\"}\n");return;
    }
    memset(&in,0,sizeof(in));
    memset(&out,0,sizeof(out));
    snprintf(in.channel_id,sizeof(in.channel_id),"%s",id);
    if(!interface_invoke(DIGIT_CHANNEL_SERVICE_GET,&in,sizeof(in),
                         &out,sizeof(out),&used) || used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"channel service unavailable\"}\n");
        return;
    }
    if(!digit_interface_channel_exact_valid(&out.channel,out.found,id)){
        interface_reply(client,503,"{\"error\":\"invalid channel response\"}\n");
        return;
    }
    if(!out.found){
        interface_reply(client,404,"{\"found\":false}\n");return;
    }
    if(!interface_channel_json(&out.channel,item,sizeof(item))){
        interface_reply(client,503,"{\"error\":\"channel serialization failed\"}\n");
        return;
    }
    n=snprintf(json,sizeof(json),"{\"found\":true,\"channel\":%s}\n",item);
    if(n<0 || (size_t)n>=sizeof(json)){
        interface_reply(client,503,"{\"error\":\"channel serialization failed\"}\n");
        return;
    }
    interface_reply(client,200,json);
}
/* [AI:GPT-6 | 2026-10-08] 1.4.7: validate the complete
 * Core message list before serializing; never silently truncate JSON. */
/* [AI:GPT-6 | 2026-10-09] Read-only operational Alerts projection.
 * The Core Alert record is authoritative: no duplicate messages, no
 * replay on refresh. This route is restricted to the STN-Labz Operations
 * Alerts binding AND a currently authorized organization SA. */
static int interface_alert_channel_messages(int client,const char *channel,
 const char *identity){
    char bound[DIGIT_CHANNEL_ID_MAX];
    digit_alert_list_request_t request;
    digit_alert_list_response_t result;
    char json[INTERFACE_BUFFER_MAX];
    size_t used=0,off=0,i;
    int n;
    if(!digit_alerts_channel_lookup(DIGIT_PROJECT_ROOT,"stn-labz","operations",
                                   bound,sizeof(bound)) ||
       strcmp(bound,channel)!=0)return 0;
    if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,"stn-labz",identity) ||
       !digit_project_security_member(DIGIT_PROJECT_ROOT,"stn-labz","operations",identity)){
        interface_reply(client,403,"{\"error\":\"alerts scope forbidden\"}\n");return 1;
    }
    memset(&request,0,sizeof(request));
    memset(&result,0,sizeof(result));
    if(!interface_invoke(DIGIT_ALERT_SERVICE_LIST,&request,sizeof(request),
                         &result,sizeof(result),&used)||used!=sizeof(result)||
       !digit_interface_alerts_valid(result.alerts,result.count,
                                    DIGIT_CORE_SERVICE_ALERT_LIST_MAX,0)){
        interface_reply(client,503,"{\"error\":\"alert records unavailable\"}\n");return 1;
    }
    n=snprintf(json,sizeof(json),"{\"channel_id\":\"%s\",\"count\":%zu,\"messages\":[",
               channel,result.count);
    if(n<0||(size_t)n>=sizeof(json))goto invalid;
    off=(size_t)n;
    for(i=0;i<result.count;++i){
        char id[160],detail[2400],summary[700],source[200],content[4096],escaped[8200];
        const digit_alert_t *a=&result.alerts[i];
        interface_json_escape(a->id,id,sizeof(id));
        interface_json_escape(a->summary,summary,sizeof(summary));
        interface_json_escape(a->source,source,sizeof(source));
        interface_json_escape(a->detail,detail,sizeof(detail));
        n=snprintf(content,sizeof(content),"[%s] %s | Source: %s | Time: %llu | ID: %s%s%s%s",
                   interface_alert_severity_string(a->severity),summary,
                   source[0]?source:"unspecified",a->created_at,id,
                   detail[0]?" | ":"",detail,a->acknowledged?" [ACK]":"");
        if(n<0||(size_t)n>=sizeof(content))goto invalid;
        interface_json_escape(content,escaped,sizeof(escaped));
        n=snprintf(json+off,sizeof(json)-off,
          "%s{\"id\":\"%s\",\"channel_id\":\"%s\",\"created_at\":%llu,\"origin\":\"digit\",\"body\":\"%s\"}",
          i?",":"",id,channel,a->created_at,escaped);
        if(n<0||(size_t)n>=sizeof(json)-off)goto invalid;
        off+=(size_t)n;
    }
    n=snprintf(json+off,sizeof(json)-off,"]}\n");
    if(n<0||(size_t)n>=sizeof(json)-off)goto invalid;
    interface_reply(client,200,json);return 1;
invalid:
    interface_reply(client,503,"{\"error\":\"Alerts channel response exceeded limits\"}\n");return 1;
}
static void interface_messages_list(int client,const char *id)
{
    digit_channel_message_list_request_t in;
    digit_channel_message_list_response_t out;
    size_t used=0,off=0,i;
    char json[INTERFACE_BUFFER_MAX];
    int n;
    memset(&out,0,sizeof(out));
    memset(&in,0,sizeof(in));
    if(!digit_controlled_message_valid(id,"digit","x")){
        interface_reply(client,400,"{\"error\":\"invalid channel id\"}\n");
        return;
    }
    snprintf(in.channel_id,sizeof(in.channel_id),"%s",id);
    if(!interface_invoke(DIGIT_CHANNEL_SERVICE_MESSAGE_LIST,
                         &in,sizeof(in),&out,sizeof(out),&used) ||
       used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"channel message service unavailable\"}\n");
        return;
    }
    if(!digit_interface_messages_valid(out.messages,out.count,
                                       DIGIT_CORE_SERVICE_MESSAGE_LIST_MAX,id)){
        interface_reply(client,503,"{\"error\":\"invalid channel messages\"}\n");
        return;
    }
    n=snprintf(json,sizeof(json),
               "{\"channel_id\":\"%s\",\"count\":%zu,\"messages\":[",id,out.count);
    if(n<0 || (size_t)n>=sizeof(json))goto invalid;
    off=(size_t)n;
    for(i=0;i<out.count;++i){
        char item[9000];
        if(!interface_message_json(&out.messages[i],item,sizeof(item)))goto invalid;
        n=snprintf(json+off,sizeof(json)-off,"%s%s",i?",":"",item);
        if(n<0 || (size_t)n>=sizeof(json)-off)goto invalid;
        off+=(size_t)n;
    }
    n=snprintf(json+off,sizeof(json)-off,"]}\n");
    if(n<0 || (size_t)n>=sizeof(json)-off)goto invalid;
    interface_reply(client,200,json);
    return;
invalid:
    interface_reply(client,503,"{\"error\":\"channel message serialization failed\"}\n");
}
static int interface_message_append(const char *id,const char *origin,const char *body,digit_channel_message_t *message){digit_channel_message_append_request_t in;digit_channel_message_append_response_t out;size_t used=0;if(!digit_controlled_message_valid(id,origin,body))return 0;memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));snprintf(in.channel_id,sizeof(in.channel_id),"%s",id);snprintf(in.origin,sizeof(in.origin),"%s",origin);snprintf(in.body,sizeof(in.body),"%s",body);if(!interface_invoke(DIGIT_CHANNEL_SERVICE_MESSAGE_APPEND,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)||!out.appended)return 0;if(message)*message=out.message;return 1;}
static void interface_message_post(int client,const char *id,const char *body,const char *identity){digit_channel_message_t message;char origin[DIGIT_CHANNEL_ORIGIN_MAX],item[9000],json[9300];if(!digit_message_origin_from_identity(identity,origin,sizeof(origin))){interface_reply(client,403,"{\"error\":\"identity cannot be represented by channel protocol\"}\n");return;}if(!interface_message_append(id,origin,body,&message)){interface_reply(client,400,"{\"error\":\"message not appended\"}\n");return;}interface_message_json(&message,item,sizeof(item));{char receipt[256];if(!digit_controlled_receipt(&message,id,origin,receipt,sizeof(receipt))){interface_reply(client,503,"{\"error\":\"Core acknowledgement inconsistent\"}\n");return;}snprintf(json,sizeof(json),"{\"appended\":true,\"receipt\":%s,\"message\":%s}\n",receipt,item);}interface_reply(client,200,json);}
static void interface_channel_ask(int client,const char *id,const char *body,const char *identity){interface_dispatcher_result_t out;interface_builder_result_t learned;digit_channel_message_t operator_message,digit_message;char answer[768],escaped[8192],er[512],eid[160],json[11000];const char *learn;char origin[DIGIT_CHANNEL_ORIGIN_MAX];if(!digit_message_origin_from_identity(identity,origin,sizeof(origin))){interface_reply(client,403,"{\"error\":\"identity cannot be represented by channel protocol\"}\n");return;}if(!body||!body[0]||strlen(body)>=DISPATCHER_REQUEST_MAX){interface_reply(client,400,"{\"error\":\"valid question required\"}\n");return;}if(!interface_message_append(id,origin,body,&operator_message)){interface_reply(client,404,"{\"error\":\"channel unavailable\"}\n");return;}learn=interface_learn_text(body);if(learn){int result=interface_process_learn(learn,&learned,answer,sizeof(answer));if(result<=0){interface_reply(client,result==0?400:503,result==0?"{\"error\":\"valid learning input required\"}\n":"{\"error\":\"corpus builder unavailable\"}\n");return;}if(!interface_message_append(id,"digit",answer,&digit_message)){interface_reply(client,503,"{\"error\":\"learning completed but channel persistence failed\"}\n");return;}interface_json_escape(answer,escaped,sizeof(escaped));interface_json_escape(learned.reason,er,sizeof(er));interface_json_escape(learned.record_id,eid,sizeof(eid));snprintf(json,sizeof(json),"{\"channel_id\":\"%s\",\"answered\":true,\"evidence_count\":0,\"answer\":\"%s\",\"learning\":true,\"stored\":%s,\"record_id\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",id,escaped,learned.stored?"true":"false",eid,learned.category,learned.confidence,er);interface_reply(client,200,json);return;}interface_dispatch(body,&out);if(out.answer[0]&&!interface_message_append(id,"digit",out.answer,&digit_message)){snprintf(out.answer,sizeof(out.answer),"I completed the request, but I couldn't persist my response to this channel.");}interface_json_escape(out.answer,escaped,sizeof(escaped));snprintf(json,sizeof(json),"{\"channel_id\":\"%s\",\"answered\":%s,\"evidence_count\":0,\"answer\":\"%s\"}\n",id,out.answered?"true":"false",escaped);interface_reply(client,200,json);}
/* [AI:GPT-6 | 2026-10-08] 1.4.8: fail closed on Core
 * alert-list count, identity, provenance, and serialization errors. */
static void interface_alerts_list(int client,int unacknowledged)
{
    digit_alert_list_request_t in;
    digit_alert_list_response_t out;
    size_t used=0,off=0,i;
    char json[INTERFACE_BUFFER_MAX];
    int written;
    memset(&in,0,sizeof(in));
    memset(&out,0,sizeof(out));
    in.unacknowledged_only=unacknowledged;
    if(!interface_invoke(DIGIT_ALERT_SERVICE_LIST,&in,sizeof(in),
                         &out,sizeof(out),&used) || used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"alert service unavailable\"}\n");
        return;
    }
    if(!digit_interface_alerts_valid(out.alerts,out.count,
                                     DIGIT_CORE_SERVICE_ALERT_LIST_MAX,
                                     unacknowledged)){
        interface_reply(client,503,"{\"error\":\"invalid alert list\"}\n");
        return;
    }
    written=snprintf(json,sizeof(json),
                     "{\"count\":%zu,\"unacknowledged_only\":%s,\"alerts\":[",
                     out.count,unacknowledged?"true":"false");
    if(written<0 || (size_t)written>=sizeof(json))goto invalid;
    off=(size_t)written;
    for(i=0;i<out.count;++i){
        char item[4000];
        if(!interface_alert_json(&out.alerts[i],item,sizeof(item)))goto invalid;
        written=snprintf(json+off,sizeof(json)-off,"%s%s",i?",":"",item);
        if(written<0 || (size_t)written>=sizeof(json)-off)goto invalid;
        off+=(size_t)written;
    }
    written=snprintf(json+off,sizeof(json)-off,"]}\n");
    if(written<0 || (size_t)written>=sizeof(json)-off)goto invalid;
    interface_reply(client,200,json);
    return;
invalid:
    interface_reply(client,503,"{\"error\":\"alert list serialization failed\"}\n");
}
/* [AI:GPT-6 | 2026-10-08] 1.4.9: untrusted Core
 * exact alert results must match requested identities and state. */
static void interface_alert_get(int client,const char *id)
{
    digit_alert_get_request_t in;
    digit_alert_get_response_t out;
    size_t used=0;
    char item[4000],json[4300];
    int n;
    if(!digit_interface_alert_exact_valid(NULL,0,id,0)){
        interface_reply(client,400,"{\"error\":\"invalid alert id\"}\n");return;
    }
    memset(&in,0,sizeof(in));
    memset(&out,0,sizeof(out));
    snprintf(in.alert_id,sizeof(in.alert_id),"%s",id);
    if(!interface_invoke(DIGIT_ALERT_SERVICE_GET,&in,sizeof(in),
                         &out,sizeof(out),&used)||used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"alert service unavailable\"}\n");return;
    }
    if(!digit_interface_alert_exact_valid(&out.alert,out.found,id,0)){
        interface_reply(client,503,"{\"error\":\"invalid alert response\"}\n");return;
    }
    if(!out.found){
        interface_reply(client,404,"{\"found\":false}\n");return;
    }
    if(!interface_alert_json(&out.alert,item,sizeof(item))){
        interface_reply(client,503,"{\"error\":\"alert serialization failed\"}\n");return;
    }
    n=snprintf(json,sizeof(json),"{\"found\":true,\"alert\":%s}\n",item);
    if(n<0||(size_t)n>=sizeof(json)){
        interface_reply(client,503,"{\"error\":\"alert serialization failed\"}\n");return;
    }
    interface_reply(client,200,json);
}
static void interface_alert_ack(int client,const char *id)
{
    digit_alert_acknowledge_request_t in;
    digit_alert_acknowledge_response_t out;
    size_t used=0;
    char item[4000],json[4300];
    int n;
    if(!digit_interface_alert_exact_valid(NULL,0,id,1)){
        interface_reply(client,400,"{\"error\":\"invalid alert id\"}\n");return;
    }
    memset(&in,0,sizeof(in));
    memset(&out,0,sizeof(out));
    snprintf(in.alert_id,sizeof(in.alert_id),"%s",id);
    if(!interface_invoke(DIGIT_ALERT_SERVICE_ACKNOWLEDGE,&in,sizeof(in),
                         &out,sizeof(out),&used)||used!=sizeof(out)){
        interface_reply(client,503,"{\"error\":\"alert service unavailable\"}\n");return;
    }
    if(!digit_interface_alert_exact_valid(&out.alert,out.acknowledged,id,1)){
        interface_reply(client,503,"{\"error\":\"invalid acknowledgement\"}\n");return;
    }
    if(!out.acknowledged){
        interface_reply(client,404,"{\"acknowledged\":false}\n");return;
    }
    if(!interface_alert_json(&out.alert,item,sizeof(item))){
        interface_reply(client,503,"{\"error\":\"alert serialization failed\"}\n");return;
    }
    n=snprintf(json,sizeof(json),"{\"acknowledged\":true,\"alert\":%s}\n",item);
    if(n<0||(size_t)n>=sizeof(json)){
        interface_reply(client,503,"{\"error\":\"alert serialization failed\"}\n");return;
    }
    interface_reply(client,200,json);
}

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
/* [AI:GPT-6 | 2026-10-08] Interface 1.5.4: protected SA admission.
 * Core decides authority against the current operator-controlled roster.
 * No administration data is exposed until Core returns an exact grant. */
if(strncmp(request,"GET /admin/access HTTP/1.1\r\n",sizeof("GET /admin/access HTTP/1.1\r\n")-1U)==0){
    digit_admin_sa_request_t in;
    digit_admin_sa_response_t out;
    size_t used=0;
    memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));
    if(strlen(identity)>=sizeof(in.identity)){
        interface_reply(client,403,"{\"error\":\"forbidden\"}\n");return;
    }
    snprintf(in.identity,sizeof(in.identity),"%s",identity);
    if(!interface_invoke(DIGIT_ADMIN_SA_SERVICE,&in,sizeof(in),&out,sizeof(out),&used) ||
       used!=sizeof(out) || out.authorized!=1){
        interface_reply(client,403,"{\"error\":\"forbidden\"}\n");return;
    }
    interface_reply(client,200,"{\"authorized\":true,\"scope\":\"digit-operations-read\"}\n");return;
}
/* [AI:GPT-6 | 2026-10-08] Interface 1.5.6: read-only SA dashboard.
 * Identity comes exclusively from the validated session; Core rechecks SA.
 * All operational counts are obtained from existing Core service contracts.
 * No filesystem, process internals, or privileged controls are exposed. */
if(strncmp(request,"GET /admin/dashboard HTTP/1.1\r\n",sizeof("GET /admin/dashboard HTTP/1.1\r\n")-1U)==0){
    digit_admin_sa_request_t auth;
    digit_admin_sa_response_t grant;
    digit_channel_list_response_t *channels=NULL;
    digit_alert_list_request_t filter={0};
    digit_alert_list_response_t *alerts=NULL;
    size_t used=0,open_alerts=0,i;
    char json[320];
    memset(&auth,0,sizeof(auth));memset(&grant,0,sizeof(grant));
    if(strlen(identity)>=sizeof(auth.identity)){
        interface_reply(client,403,"{\"error\":\"forbidden\"}\n");return;
    }
    snprintf(auth.identity,sizeof(auth.identity),"%s",identity);
    if(!interface_invoke(DIGIT_ADMIN_SA_SERVICE,&auth,sizeof(auth),&grant,sizeof(grant),&used)||
       used!=sizeof(grant)||grant.authorized!=1){
        interface_reply(client,403,"{\"error\":\"forbidden\"}\n");return;
    }
    channels=calloc(1,sizeof(*channels));
    alerts=calloc(1,sizeof(*alerts));
    if(!channels||!alerts){free(channels);free(alerts);interface_reply(client,503,"{\"error\":\"dashboard unavailable\"}\n");return;}
    used=0;
    if(!interface_invoke(DIGIT_CHANNEL_SERVICE_LIST,NULL,0,channels,sizeof(*channels),&used)||
       used!=sizeof(*channels)||channels->count>DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX){
        free(channels);free(alerts);interface_reply(client,503,"{\"error\":\"channel snapshot unavailable\"}\n");return;
    }
    used=0;
    if(!interface_invoke(DIGIT_ALERT_SERVICE_LIST,&filter,sizeof(filter),alerts,sizeof(*alerts),&used)||
       used!=sizeof(*alerts)||alerts->count>DIGIT_CORE_SERVICE_ALERT_LIST_MAX){
        free(channels);free(alerts);interface_reply(client,503,"{\"error\":\"alert snapshot unavailable\"}\n");return;
    }
    for(i=0;i<alerts->count;++i)if(!alerts->alerts[i].acknowledged)++open_alerts;
    if(!digit_dashboard_response(grant.authorized,channels->count,alerts->count,open_alerts,
          DIGIT_CORE_SERVICE_CHANNEL_LIST_MAX,DIGIT_CORE_SERVICE_ALERT_LIST_MAX,json,sizeof(json))){
        free(channels);free(alerts);interface_reply(client,503,"{\"error\":\"dashboard snapshot invalid\"}\n");return;
    }
    free(channels);free(alerts);
    interface_reply(client,200,json);return;
}
/* [AI:GPT-6 | 2026-10-09] Atomic authority decisions, staged
 * resources: one SA action provisions, binds and self-grants Security.
 * Retry continues completed stages; authorization is rechecked at every
 * boundary. A partial failure is reported, never presented as success. */
/* [AI:GPT-6 | 2026-10-09] Strict same-organization SA roster.
 * Role assignment requires pre-existing qualifications and current SA. */
/* [AI:GPT-6 | 2026-10-09] SA-authorized restricted project member directory. */
if(strncmp(request,"POST /admin/project-members HTTP/1.1\r\n",
 sizeof("POST /admin/project-members HTTP/1.1\r\n")-1U)==0){
 char org[64],project[64],json[16384];const char *tab;
 size_t on,pn,i;
 if(!body||(tab=strchr(body,'\t'))==NULL||strchr(tab+1,'\t')||
    strchr(body,'\r')||strchr(body,'\n')){
  interface_reply(client,400,"{\"error\":\"invalid member scope\"}\n");return;
 }
 on=(size_t)(tab-body);pn=strlen(tab+1);
 if(!on||!pn||on>=sizeof(org)||pn>=sizeof(project)){
  interface_reply(client,400,"{\"error\":\"invalid member scope\"}\n");return;
 }
 for(i=0;i<on+pn;i++){
  unsigned char c=(unsigned char)(i<on?body[i]:tab[1+i-on]);
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
       (c>='0'&&c<='9')||c=='-'||c=='_')){
   interface_reply(client,400,"{\"error\":\"invalid member scope\"}\n");return;
  }
 }
 memcpy(org,body,on);org[on]=0;memcpy(project,tab+1,pn);project[pn]=0;
 if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,org,identity)||
    !digit_project_security_member(DIGIT_PROJECT_ROOT,org,project,identity)){
  interface_reply(client,403,"{\"error\":\"project SA membership required\"}\n");return;
 }
 if(!digit_project_members_json(DIGIT_PROJECT_ROOT,org,project,identity,
      DIGIT_SECURITY_SA_REGISTRY,json,sizeof(json))){
  interface_reply(client,503,"{\"error\":\"project member directory unavailable\"}\n");return;
 }
 interface_reply(client,200,json);return;
}
if(strncmp(request,"POST /admin/sa HTTP/1.1\r\n",sizeof("POST /admin/sa HTTP/1.1\r\n")-1U)==0){
    char command[12],org[64],user[64],json[8192];
    char *one,*two;
    size_t a,b,c,i;
    if(!body||!(one=strchr(body,'\t'))||!(two=strchr(one+1,'\t'))||
       strchr(two+1,'\t')||strchr(body,'\r')||strchr(body,'\n')){
        interface_reply(client,400,"{\"error\":\"invalid SA command\"}\n");return;
    }
    a=(size_t)(one-body);b=(size_t)(two-one-1);c=strlen(two+1);
    if(!a||a>=sizeof(command)||!b||b>=sizeof(org)||c>=sizeof(user)){
        interface_reply(client,400,"{\"error\":\"invalid SA arguments\"}\n");return;
    }
    memcpy(command,body,a);command[a]=0;
    memcpy(org,one+1,b);org[b]=0;
    memcpy(user,two+1,c+1);
    for(i=0;i<b+c;i++){
        char ch=i<b?org[i]:user[i-b];
        if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
             (ch>='0'&&ch<='9')||ch=='-'||ch=='_'||ch=='.')){
            interface_reply(client,400,"{\"error\":\"invalid SA scope\"}\n");return;
        }
    }
    if(!strcmp(command,"bootstrap")&&c){
        if(!digit_sa_change(DIGIT_SECURITY_SA_REGISTRY,org,identity,user,2)){
            interface_reply(client,403,"{\"error\":\"bootstrap denied; verify existing qualification, empty target SA roster and founder assignment\"}\n");return;
        }
        interface_reply(client,200,"{\"bootstrapped\":true}\n");return;
    }
    if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,org,identity)){
        interface_reply(client,403,"{\"error\":\"organization SA required\"}\n");return;
    }
    if(!strcmp(command,"list")&&!c){
        if(!digit_sa_list(DIGIT_SECURITY_SA_REGISTRY,org,identity,json,sizeof(json))){
            interface_reply(client,503,"{\"error\":\"SA registry unavailable\"}\n");return;
        }
        interface_reply(client,200,json);return;
    }
    if((!strcmp(command,"assign")||!strcmp(command,"revoke"))&&c){
        if(!digit_sa_change(DIGIT_SECURITY_SA_REGISTRY,org,identity,user,!strcmp(command,"assign"))){
            interface_reply(client,403,"{\"error\":\"SA change denied or target not qualified\"}\n");return;
        }
        interface_reply(client,200,"{\"updated\":true}\n");return;
    }
    interface_reply(client,400,"{\"error\":\"unknown SA command\"}\n");return;
}
if(strncmp(request,"POST /admin/security/setup HTTP/1.1\r\n",
           sizeof("POST /admin/security/setup HTTP/1.1\r\n")-1U)==0){
    char org[DIGIT_PROJECT_ID_MAX],project[DIGIT_PROJECT_ID_MAX];
    char channel[DIGIT_CHANNEL_ID_MAX],json[256];
    digit_grant_request_t access;
    const char *sep;
    size_t on,pn,i;
    if(!body||!(sep=strchr(body,'\t'))||strchr(sep+1,'\t')){
        interface_reply(client,400,"{\"error\":\"expected organization TAB project\"}\n");return;
    }
    on=(size_t)(sep-body);pn=strlen(sep+1);
    if(!on||!pn||on>=sizeof(org)||pn>=sizeof(project)){
        interface_reply(client,400,"{\"error\":\"invalid scope\"}\n");return;
    }
    for(i=0;i<on+pn;++i){
        unsigned char c=(unsigned char)(i<on?body[i]:sep[1+i-on]);
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_')){
            interface_reply(client,400,"{\"error\":\"invalid scope\"}\n");return;
        }
    }
    memcpy(org,body,on);org[on]=0;
    memcpy(project,sep+1,pn);project[pn]=0;
    if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,org,identity)){
        interface_reply(client,403,"{\"error\":\"organization SA assignment required\"}\n");return;
    }
    if(!digit_project_security_ready(DIGIT_PROJECT_ROOT,org,project) &&
       !digit_project_provision(DIGIT_PROJECT_ROOT,org,project,identity,identity,
                                DIGIT_SECURITY_SA_REGISTRY)){
        interface_reply(client,503,"{\"error\":\"project setup incomplete\"}\n");return;
    }
    if(!digit_project_security_member(DIGIT_PROJECT_ROOT,org,project,identity)){
        interface_reply(client,403,"{\"error\":\"project membership required\"}\n");return;
    }
    if(!digit_project_security_channel_id(DIGIT_PROJECT_ROOT,org,project,channel,sizeof(channel))){
        if(!interface_host || !digit_project_bind_security_host(DIGIT_PROJECT_ROOT,
            org,project,identity,DIGIT_SECURITY_SA_REGISTRY,interface_host) ||
           !digit_project_security_channel_id(DIGIT_PROJECT_ROOT,org,project,channel,sizeof(channel))){
            interface_reply(client,503,"{\"error\":\"security binding incomplete\"}\n");return;
        }
    }
    if(!digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,identity,channel)){
        memset(&access,0,sizeof(access));
        snprintf(access.organization,sizeof(access.organization),"%s",org);
        snprintf(access.project,sizeof(access.project),"%s",project);
        snprintf(access.channel,sizeof(access.channel),"%s",channel);
        snprintf(access.user,sizeof(access.user),"%s",identity);
        if(!digit_grant_security_scoped(DIGIT_CHANNEL_ACL_PATH,DIGIT_PROJECT_ROOT,
               DIGIT_SECURITY_SA_REGISTRY,identity,&access)){
            interface_reply(client,503,"{\"error\":\"security grant incomplete\"}\n");return;
        }
    }
    if(!digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,identity,channel)){
        interface_reply(client,503,"{\"error\":\"security access not verified\"}\n");return;
    }
    /* [AI:GPT-6 | 2026-10-09] One SA action guarantees both
     * required administrative channels, with separate explicit grants. */
    {
        char alerts_id[DIGIT_CHANNEL_ID_MAX];
        if(!digit_alerts_channel_ensure(DIGIT_PROJECT_ROOT,org,project,identity,
               DIGIT_SECURITY_SA_REGISTRY,interface_host,alerts_id,sizeof(alerts_id))){
            interface_reply(client,503,"{\"error\":\"Alerts provisioning incomplete\"}\n");return;
        }
        if(!digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,identity,alerts_id)){
            memset(&access,0,sizeof(access));
            snprintf(access.organization,sizeof(access.organization),"%s",org);
            snprintf(access.project,sizeof(access.project),"%s",project);
            snprintf(access.channel,sizeof(access.channel),"%s",alerts_id);
            snprintf(access.user,sizeof(access.user),"%s",identity);
            if(!digit_grant_alerts_scoped(DIGIT_CHANNEL_ACL_PATH,DIGIT_PROJECT_ROOT,
                   DIGIT_SECURITY_SA_REGISTRY,identity,&access) ||
               !digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,identity,alerts_id)){
                interface_reply(client,503,"{\"error\":\"Alerts authorization incomplete\"}\n");return;
            }
        }
        snprintf(json,sizeof(json),
            "{\"ready\":true,\"security_id\":\"%s\",\"alerts_id\":\"%s\"}\n",
            channel,alerts_id);
    }

    interface_reply(client,200,json);return;
}
/* [AI:GPT-6 | 2026-10-09] Return the exact bound Security ID only
 * to a currently assigned organization SA who belongs to the project.
 * This is metadata retrieval, not a new authorization grant. */
if(strncmp(request,"GET /admin/security-binding?scope=",
           sizeof("GET /admin/security-binding?scope=")-1U)==0){
    const char *begin=request+sizeof("GET /admin/security-binding?scope=")-1U;
    const char *end=strchr(begin,' ');
    const char *sep;
    char org[DIGIT_PROJECT_ID_MAX],project[DIGIT_PROJECT_ID_MAX];
    char channel[DIGIT_CHANNEL_ID_MAX],json[256];
    size_t on,pn,i;
    if(!end||strncmp(end," HTTP/1.1\r\n",sizeof(" HTTP/1.1\r\n")-1U)!=0||
       !(sep=memchr(begin,'/',(size_t)(end-begin)))){
        interface_reply(client,400,"{\"error\":\"invalid project scope\"}\n");return;
    }
    on=(size_t)(sep-begin);pn=(size_t)(end-sep-1);
    if(!on||!pn||on>=sizeof(org)||pn>=sizeof(project)){
        interface_reply(client,400,"{\"error\":\"invalid project scope\"}\n");return;
    }
    for(i=0;i<on+pn;++i){
        unsigned char c=(unsigned char)(i<on?begin[i]:sep[1+i-on]);
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_')){
            interface_reply(client,400,"{\"error\":\"invalid project scope\"}\n");return;
        }
    }
    memcpy(org,begin,on);org[on]=0;
    memcpy(project,sep+1,pn);project[pn]=0;
    if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,org,identity)||
       !digit_project_security_member(DIGIT_PROJECT_ROOT,org,project,identity)){
        interface_reply(client,403,"{\"error\":\"organization SA and project membership required\"}\n");return;
    }
    if(!digit_project_security_channel_id(DIGIT_PROJECT_ROOT,org,project,channel,sizeof(channel))){
        interface_reply(client,404,"{\"error\":\"security channel not bound\"}\n");return;
    }
    snprintf(json,sizeof(json),"{\"channel_id\":\"%s\"}\n",channel);
    interface_reply(client,200,json);return;
}
/* [AI:GPT-6 | 2026-10-09] 1.6.0 restricted SA channel grant.
 * The authenticated identity, not the request body, is the granting actor.
 * No grant exists unless the project Security channel is already bound. */
if(strncmp(request,"POST /admin/security-grants HTTP/1.1\r\n",
           sizeof("POST /admin/security-grants HTTP/1.1\r\n")-1U)==0){
    digit_grant_request_t grant_request;
    if(!body||!digit_grant_request_parse(body,&grant_request)){
        interface_reply(client,400,"{\"error\":\"invalid grant request\"}\n");return;
    }
    if(!digit_grant_security_scoped(DIGIT_CHANNEL_ACL_PATH,DIGIT_PROJECT_ROOT,
          DIGIT_SECURITY_SA_REGISTRY,identity,&grant_request)){
        interface_reply(client,403,"{\"error\":\"grant denied or registry unavailable\"}\n");return;
    }
    interface_reply(client,200,"{\"granted\":true,\"scope\":\"security\"}\n");return;
}
/* [AI:GPT-6 | 2026-10-09] 1.5.10: SA-only project inventory.
 * No cross-organization fallback: each explicit organization must be
 * authorized independently against the current protected roster. */
if(strncmp(request,"GET /admin/projects?organization=",
           sizeof("GET /admin/projects?organization=")-1U)==0){
    const char *start=request+sizeof("GET /admin/projects?organization=")-1U;
    const char *end=strchr(start,' ');
    char org[DIGIT_PROJECT_ID_MAX],json[8192];
    size_t n;
    if(!end||strncmp(end," HTTP/1.1\r\n",sizeof(" HTTP/1.1\r\n")-1U)!=0){
        interface_reply(client,400,"{\"error\":\"invalid project list request\"}\n");return;
    }
    n=(size_t)(end-start);
    if(n==0||n>=sizeof(org)){interface_reply(client,400,"{\"error\":\"invalid organization\"}\n");return;}
    memcpy(org,start,n);org[n]=0;
    if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,org,identity)){
        interface_reply(client,403,"{\"error\":\"organization SA assignment required\"}\n");return;
    }
    if(!digit_project_list_scoped(DIGIT_PROJECT_ROOT,org,identity,json,sizeof(json))){
        interface_reply(client,503,"{\"error\":\"project inventory unavailable\"}\n");return;
    }
    interface_reply(client,200,json);return;
}
/* [AI:GPT-6 | 2026-10-09] Interface 1.5.9: authenticated, scoped
 * SA project provisioning. The session identity is the project founder;
 * the organization is checked against the protected SA roster. */
if(strncmp(request,"POST /admin/projects HTTP/1.1\r\n",
           sizeof("POST /admin/projects HTTP/1.1\r\n")-1U)==0){
    char organization[DIGIT_PROJECT_ID_MAX],project[DIGIT_PROJECT_ID_MAX];
    const char *separator;
    size_t org_length,project_length,i;
    if(!body || !(separator=strchr(body,'\t')) || strchr(separator+1,'\t')){
        interface_reply(client,400,"{\"error\":\"expected organization TAB project\"}\n");return;
    }
    org_length=(size_t)(separator-body);
    project_length=strlen(separator+1);
    if(org_length==0 || project_length==0 ||
       org_length>=sizeof(organization) || project_length>=sizeof(project)){
        interface_reply(client,400,"{\"error\":\"invalid project scope\"}\n");return;
    }
    for(i=0;i<org_length+project_length;++i){
        const unsigned char c=(unsigned char)(i<org_length?body[i]:separator[1+i-org_length]);
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='-'||c=='_')){
            interface_reply(client,400,"{\"error\":\"invalid project identifier\"}\n");return;
        }
    }
    memcpy(organization,body,org_length);organization[org_length]=0;
    memcpy(project,separator+1,project_length);project[project_length]=0;
    if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,organization,identity)){
        interface_reply(client,403,"{\"error\":\"organization SA assignment required\"}\n");return;
    }
    if(!digit_project_provision(DIGIT_PROJECT_ROOT,organization,project,
                                identity,identity,DIGIT_SECURITY_SA_REGISTRY)){
        interface_reply(client,503,"{\"error\":\"project provisioning failed or already exists\"}\n");return;
    }
    interface_reply(client,200,"{\"created\":true,\"security_ready\":true,\"channel_bound\":false}\n");return;
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
/* [AI:GPT-6 | 2026-10-09] Project-scoped, end-to-end ordinary channel
 * creation. Core creation by itself is never reported as ready. */
if(strncmp(request,"POST /admin/channels HTTP/1.1\r\n",
 sizeof("POST /admin/channels HTTP/1.1\r\n")-1U)==0){
 char org[64],project[64],name[64],channel[DIGIT_CHANNEL_ID_MAX],json[256];
 char *a,*b,*parts[3];size_t i,j,n;
 digit_grant_request_t access;
 if(!body||(a=strchr(body,'\t'))==NULL||
    (b=strchr(a+1,'\t'))==NULL||strchr(b+1,'\t')||
    strchr(body,'\r')||strchr(body,'\n')){
  interface_reply(client,400,"{\"error\":\"expected organization, project and channel name\"}\n");return;
 }
 parts[0]=org;parts[1]=project;parts[2]=name;
 {const char *begins[3]={body,a+1,b+1};const char *ends[3]={a,b,body+strlen(body)};
  for(i=0;i<3;i++){
   n=(size_t)(ends[i]-begins[i]);
   if(!n||n>=64||(i==2&&n>40)){
    interface_reply(client,400,"{\"error\":\"invalid project channel scope\"}\n");return;
   }
   for(j=0;j<n;j++){
    unsigned char c=(unsigned char)begins[i][j];
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
         (c>='0'&&c<='9')||c=='-'||c=='_')){
     interface_reply(client,400,"{\"error\":\"invalid channel identifier\"}\n");return;
    }
   }
   memcpy(parts[i],begins[i],n);parts[i][n]=0;
  }
 }
 if(!digit_security_sa_verify(DIGIT_SECURITY_SA_REGISTRY,org,identity)||
    !digit_project_security_ready(DIGIT_PROJECT_ROOT,org,project)||
    !digit_project_security_member(DIGIT_PROJECT_ROOT,org,project,identity)){
  interface_reply(client,403,"{\"error\":\"existing project SA membership required\"}\n");return;
 }
 if(!interface_host||!digit_project_channel_create_host(DIGIT_PROJECT_ROOT,
    org,project,name,identity,DIGIT_SECURITY_SA_REGISTRY,interface_host,
    channel,sizeof(channel))){
  interface_reply(client,503,"{\"error\":\"project channel binding incomplete\"}\n");return;
 }
 memset(&access,0,sizeof(access));
 snprintf(access.organization,sizeof(access.organization),"%s",org);
 snprintf(access.project,sizeof(access.project),"%s",project);
 snprintf(access.channel,sizeof(access.channel),"%s",channel);
 snprintf(access.user,sizeof(access.user),"%s",identity);
 if(!digit_grant_project_channel_scoped(DIGIT_CHANNEL_ACL_PATH,
    DIGIT_PROJECT_ROOT,DIGIT_SECURITY_SA_REGISTRY,identity,&access)||
    !digit_channel_acl_check_file(DIGIT_CHANNEL_ACL_PATH,identity,channel)){
  interface_reply(client,503,"{\"error\":\"project channel access incomplete\"}\n");return;
 }
 snprintf(json,sizeof(json),"{\"ready\":true,\"organization\":\"%s\",\"project\":\"%s\",\"name\":\"%s\"}\n",org,project,name);
 interface_reply(client,200,json);return;
}
if(strncmp(request,"GET /channels ",14)==0){interface_channels_list(client,identity);return;}if(strncmp(request,"POST /channels HTTP/1.1\r\n",sizeof("POST /channels HTTP/1.1\r\n")-1U)==0){
    digit_admin_sa_request_t auth;
    digit_admin_sa_response_t grant;
    digit_channel_create_request_t create;
    digit_channel_create_response_t result;
    char json[768],item[700];
    size_t used=0;
    memset(&auth,0,sizeof(auth));memset(&grant,0,sizeof(grant));
    memset(&create,0,sizeof(create));memset(&result,0,sizeof(result));
    if(strlen(identity)>=sizeof(auth.identity)){interface_reply(client,403,"{\"error\":\"forbidden\"}\n");return;}
    snprintf(auth.identity,sizeof(auth.identity),"%s",identity);
    if(!interface_invoke(DIGIT_ADMIN_SA_SERVICE,&auth,sizeof(auth),&grant,sizeof(grant),&used)||
       used!=sizeof(grant)||grant.authorized!=1){interface_reply(client,403,"{\"error\":\"forbidden\"}\n");return;}
    if(!body||!digit_interface_channel_create_name(body,sizeof(create.name))){
        interface_reply(client,400,"{\"error\":\"invalid channel name\"}\n");return;
    }
    snprintf(create.name,sizeof(create.name),"%s",body);
    used=0;
    if(!interface_invoke(DIGIT_CHANNEL_SERVICE_CREATE,&create,sizeof(create),&result,sizeof(result),&used)||
       used!=sizeof(result)){interface_reply(client,503,"{\"error\":\"channel creation unavailable\"}\n");return;}
    if(!result.created){interface_reply(client,400,"{\"error\":\"channel creation rejected\"}\n");return;}
    if(!interface_channel_json(&result.channel,item,sizeof(item))){
        interface_reply(client,503,"{\"error\":\"channel response invalid\"}\n");return;
    }
    snprintf(json,sizeof(json),"{\"created\":true,\"acl_assigned\":false,\"channel\":%s}\n",item);
    interface_reply(client,200,json);return;
}if(strncmp(request,"GET /channels/",14)==0&&interface_path_two(request,"GET /channels/",id,sizeof(id),"/messages HTTP/1.1")){if(!interface_alert_channel_messages(client,id,identity))interface_messages_list(client,id);return;}if(strncmp(request,"POST /channels/",15)==0&&interface_path_two(request,"POST /channels/",id,sizeof(id),"/messages HTTP/1.1")){interface_message_post(client,id,body,identity);return;}if(strncmp(request,"POST /channels/",15)==0&&interface_path_two(request,"POST /channels/",id,sizeof(id),"/ask HTTP/1.1")){interface_channel_ask(client,id,body,identity);return;}if(strncmp(request,"GET /channels/",14)==0&&interface_path_value(request,"GET /channels/",id,sizeof(id))){interface_channel_get(client,id);return;}
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
    if(!digit_interface_corpus_exact_json(&out.record,out.found,id,result,sizeof(result))){
        interface_reply(client,503,"{\"error\":\"corpus response invalid\"}\n");return;
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
    if(!digit_interface_corpus_search_json(out.records,out.count,CORPUS_SEARCH_MAX,result,sizeof(result))){
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
    if(!digit_interface_corpus_exact_json(&out.record,out.found,rid,json,sizeof(json))){
        interface_reply(client,503,"{\"error\":\"invalid corpus result\"}\n");return;
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
    if(!digit_interface_corpus_search_json(out.records,out.count,CORPUS_SEARCH_MAX,json,sizeof(json))){
        interface_reply(client,503,"{\"error\":\"invalid corpus search result\"}\n");return;
    }
    interface_reply(client,200,json);return;
}
if(strncmp(request,"POST /input ",12)==0){interface_builder_request_t in;interface_builder_result_t out;size_t used=0;char er[512],eid[160],json[1200];if(!body||!body[0]||strlen(body)>=sizeof(in.text)){interface_reply(client,400,"{\"error\":\"empty or oversized input\"}\n");return;}memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));snprintf(in.text,sizeof(in.text),"%s",body);snprintf(in.source,sizeof(in.source),"interface:/input");if(!interface_invoke(CORPUS_BUILDER_SERVICE,&in,sizeof(in),&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"corpus builder unavailable\"}\n");return;}interface_json_escape(out.reason,er,sizeof(er));interface_json_escape(out.record_id,eid,sizeof(eid));snprintf(json,sizeof(json),"{\"accepted\":true,\"corpus_candidate\":%s,\"stored\":%s,\"record_id\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",out.candidate?"true":"false",out.stored?"true":"false",eid,out.category,out.confidence,er);interface_reply(client,200,json);return;}
if(strncmp(request,"POST /reason ",13)==0){interface_reasoning_result_t out;size_t used=0;char er[512],json[1024];if(!body||!body[0]){interface_reply(client,400,"{\"error\":\"empty input\"}\n");return;}memset(&out,0,sizeof(out));if(!interface_invoke(REASONING_SERVICE,body,strlen(body)+1,&out,sizeof(out),&used)||used!=sizeof(out)){interface_reply(client,503,"{\"error\":\"reasoning service unavailable\"}\n");return;}interface_json_escape(out.reason,er,sizeof(er));snprintf(json,sizeof(json),"{\"relevance\":\"%s\",\"category\":\"%s\",\"confidence\":%u,\"reason\":\"%s\"}\n",interface_relevance_string(out.relevance),interface_category_string(out.category),out.confidence,er);interface_reply(client,200,json);return;}
interface_reply(client,404,"{\"error\":\"unknown endpoint\"}\n");}

static void *interface_server(void *unused){(void)unused;while(interface_running){int client=accept(interface_fd,NULL,NULL);if(client<0){if(!interface_running)break;if(errno==EINTR)continue;continue;}{struct timeval timeout={5,0};
        (void)setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        if(interface_tls_context){
            interface_tls_client=SSL_new(interface_tls_context);
            if(interface_tls_client && SSL_set_fd(interface_tls_client,client)==1 && SSL_accept(interface_tls_client)==1)
                interface_handle(client);
            interface_audit_request=NULL;
            if(interface_tls_client){SSL_shutdown(interface_tls_client);SSL_free(interface_tls_client);interface_tls_client=NULL;}
        }else interface_handle(client);
        interface_audit_request=NULL;close(client);}}return NULL;}
/* [AI:GPT-6 | 2026-10-08] Interface 1.4.6: replace hardcoded
 * qualification counters with executable, side-effect-free checks.
 * These are module admission smoke checks, not end-to-end HTTP testing. */
static stnlabz_module_result_t interface_qualify(stnlabz_module_qualification_result_t *r)
{
    digit_knowledge_record_t record={0};
    char json[10000];
    int tests[22];
    size_t i;
    if(!r)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(r,0,sizeof(*r));
    snprintf(record.id,sizeof(record.id),"record-1");
    snprintf(record.category,sizeof(record.category),"engineering");
    snprintf(record.source,sizeof(record.source),"manual:1");
    snprintf(record.text,sizeof(record.text),"evidence");
    tests[0]=digit_knowledge_query_valid("valid query");
    tests[1]=!digit_knowledge_query_valid("");
    tests[2]=!digit_knowledge_query_valid("bad\nquery");
    tests[3]=digit_knowledge_record_id_valid(record.id);
    tests[4]=!digit_knowledge_record_id_valid("../invalid");
    tests[5]=digit_knowledge_record_valid(&record);
    tests[6]=digit_knowledge_results_valid(&record,1);
    tests[7]=digit_knowledge_result_json(&record,1,json,sizeof(json));
    tests[8]=digit_knowledge_single_json(NULL,0,json,sizeof(json));
    tests[9]=!digit_knowledge_result_json(NULL,1,json,sizeof(json));
    tests[10]=!digit_interface_http_write(-1,200,"{}");
    tests[11]=!digit_interface_http_write(-1,200,NULL);
    /* [AI:GPT-6 | 2026-10-08] 1.4.8: execute alert boundary smoke checks. */
    tests[12]=digit_interface_alerts_valid(NULL,0,0,0);
    tests[13]=!digit_interface_alerts_valid(NULL,1,0,0);
    /* [AI:GPT-6 | 2026-10-08] 1.4.10 channel boundary smoke checks. */
    tests[14]=digit_interface_channels_valid(NULL,0,0);
    tests[15]=!digit_interface_channels_valid(NULL,1,0);
    /* [AI:GPT-6 | 2026-10-08] 1.5.0 Corpus admission checks. */
    tests[16]=digit_interface_corpus_search_valid(NULL,0,16);
    tests[17]=!digit_interface_corpus_search_valid(NULL,1,16);
    tests[18]=!digit_interface_corpus_exact_valid(NULL,1,"record-1");
    /* [AI:GPT-6 | 2026-10-08] 1.5.2 executable shared-route checks. */
    tests[19]=digit_interface_corpus_exact_json(&record,1,"record-1",json,sizeof(json));
    tests[20]=!digit_interface_corpus_exact_json(&record,1,"wrong-id",json,sizeof(json));
    tests[21]=!digit_interface_corpus_search_json(&record,2,1,json,sizeof(json));
    for(i=0;i<sizeof(tests)/sizeof(tests[0]);++i){
        ++r->tests_executed;
        if(tests[i])++r->tests_passed;
        else ++r->tests_failed;
    }
    r->negative_test_executed=1;
    r->negative_test_passed=tests[1] && tests[2] && tests[4] &&
                            tests[9] && tests[10] && tests[11] && tests[13] && tests[15] &&
                            tests[17] && tests[18] && tests[20] && tests[21];
    return r->tests_failed==0 && r->negative_test_passed?
           STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
}
/* [AI:GPT-6 | 2026-10-08] TLS-only remote Interface. No plaintext
 * fallback: absent or invalid certificates prevent activation. */
static stnlabz_module_result_t interface_start(const stnlabz_module_host_t *h)
{
    struct sockaddr_in a;
    int enabled=1;
    if(!h||!h->invoke_service)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    /* Reject missing, symlinked, group-readable, or world-readable keys. */
    {
        struct stat key_status;
        if(lstat(DIGIT_INTERFACE_TLS_KEY,&key_status)!=0 ||
           !S_ISREG(key_status.st_mode) || key_status.st_nlink!=1 ||
           (key_status.st_mode&077)!=0 ||
           (key_status.st_uid!=0 && key_status.st_uid!=geteuid()))
            return STNLABZ_MODULE_ERR_START_FAILED;
    }
    interface_tls_context=SSL_CTX_new(TLS_server_method());
    if(!interface_tls_context)return STNLABZ_MODULE_ERR_START_FAILED;
    if(SSL_CTX_set_min_proto_version(interface_tls_context,TLS1_2_VERSION)!=1 ||
       SSL_CTX_use_certificate_chain_file(interface_tls_context,DIGIT_INTERFACE_TLS_CERT)!=1 ||
       SSL_CTX_use_PrivateKey_file(interface_tls_context,DIGIT_INTERFACE_TLS_KEY,SSL_FILETYPE_PEM)!=1 ||
       SSL_CTX_check_private_key(interface_tls_context)!=1){
        SSL_CTX_free(interface_tls_context);interface_tls_context=NULL;
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    digit_session_store_init(&interface_sessions);
    interface_host=h;
    interface_fd=socket(AF_INET,SOCK_STREAM,0);
    if(interface_fd<0)goto failed;
    (void)setsockopt(interface_fd,SOL_SOCKET,SO_REUSEADDR,&enabled,sizeof(enabled));
    memset(&a,0,sizeof(a));a.sin_family=AF_INET;
    a.sin_port=htons(DIGIT_INTERFACE_DEFAULT_PORT);
    if(inet_pton(AF_INET,DIGIT_INTERFACE_DEFAULT_HOST,&a.sin_addr)!=1 ||
       bind(interface_fd,(struct sockaddr *)&a,sizeof(a))!=0 || listen(interface_fd,8)!=0)
        goto failed;
    interface_running=1;
    if(pthread_create(&interface_thread,NULL,interface_server,NULL)!=0){interface_running=0;goto failed;}
    if(h->send_message)(void)h->send_message("[INTERFACE] HTTPS listener active on port 8081; TLS required");
    return STNLABZ_MODULE_OK;
failed:
    if(interface_fd>=0){close(interface_fd);interface_fd=-1;}
    SSL_CTX_free(interface_tls_context);interface_tls_context=NULL;interface_host=NULL;
    return STNLABZ_MODULE_ERR_START_FAILED;
}
static stnlabz_module_result_t interface_stop(void){if(interface_fd>=0){interface_running=0;shutdown(interface_fd,SHUT_RDWR);close(interface_fd);interface_fd=-1;(void)pthread_join(interface_thread,NULL);}digit_session_store_init(&interface_sessions);interface_host=NULL;if(interface_tls_context){SSL_CTX_free(interface_tls_context);interface_tls_context=NULL;}return STNLABZ_MODULE_OK;}
/* [AI:GPT-6 | 2026-10-08] Advertise the qualified 1.5.3 Builder response release. */
static const stnlabz_module_descriptor_t interface_descriptor={"interface","Digit Interface",1,6,9,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,interface_qualify,interface_start,interface_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &interface_descriptor;}
