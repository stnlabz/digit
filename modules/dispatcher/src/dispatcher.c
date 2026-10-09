#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "dispatcher.h"
#include "source.h"
#include "response.h"
#include "lesson.h"
#include "intent.h"
#include "interpretation.h"

/* [AI:GPT-5.6 Sol | 2026-10-06T21:41:00Z] Removed Dispatcher LLM dependency. Project questions now pass through Response so source/corpus selection and Validator remain the bounded answer path. */
/* [AI:GPT-5.6 Sol | 2026-10-06T23:28:00Z] Dispatcher now requires Intent interpretation before normal routing. Existing bounded lesson/source/action behavior is preserved while Intent becomes the authoritative request-purpose classification. */
/* [AI:GPT-5.6 Sol | 2026-10-06T23:44:00Z] Established Intent now controls downstream dispatch. Structured intent, target, and subject accompany the original request; UNKNOWN/AMBIGUOUS fail closed instead of entering generic retrieval. */
/* [AI:GPT-5.6 Sol | 2026-10-07T00:09:00Z] Removed direct linkage to Intent module implementation symbols. Dispatcher communicates with Intent only through the registered service ABI so RTLD_NOW can load Dispatcher independently. */

static const stnlabz_module_host_t *dispatcher_host=NULL;
static const char *intent_class_name(digit_intent_class_t intent){switch(intent){case DIGIT_INTENT_CONVERSATION:return "CONVERSATION";case DIGIT_INTENT_FACT:return "FACT";case DIGIT_INTENT_DEFINE:return "DEFINE";case DIGIT_INTENT_EXPLAIN:return "EXPLAIN";case DIGIT_INTENT_COMPARE:return "COMPARE";case DIGIT_INTENT_WHY:return "WHY";case DIGIT_INTENT_HOW:return "HOW";case DIGIT_INTENT_STATUS:return "STATUS";case DIGIT_INTENT_ACTION:return "ACTION";case DIGIT_INTENT_AMBIGUOUS:return "AMBIGUOUS";default:return "UNKNOWN";}}
static const char *intent_target_name(digit_intent_target_t target){switch(target){case DIGIT_INTENT_TARGET_SOCIAL:return "SOCIAL";case DIGIT_INTENT_TARGET_KNOWLEDGE:return "KNOWLEDGE";case DIGIT_INTENT_TARGET_RUNTIME:return "RUNTIME";case DIGIT_INTENT_TARGET_CAPABILITY:return "CAPABILITY";default:return "UNKNOWN";}}

static int contains_ci(const char *text,const char *needle){size_t i,j,tl,nl;if(text==NULL||needle==NULL||needle[0]=='\0')return 0;tl=strlen(text);nl=strlen(needle);if(nl>tl)return 0;for(i=0;i+nl<=tl;++i){for(j=0;j<nl;++j)if(tolower((unsigned char)text[i+j])!=tolower((unsigned char)needle[j]))break;if(j==nl)return 1;}return 0;}
static void normalize_answer(char *text){size_t i;int previous_space=0;if(text==NULL)return;for(i=0;text[i]!='\0';++i){unsigned char c=(unsigned char)text[i];if(c=='\t'||c=='\n'||c=='\r')text[i]=' ';if(text[i]==' '){if(previous_space){size_t j=i;do{text[j]=text[j+1];++j;}while(text[j-1]!='\0');--i;continue;}previous_space=1;}else previous_space=0;}}
static int append_text(char *dst,size_t dst_size,const char *src){size_t used,available,n;if(dst==NULL||dst_size==0||src==NULL)return 0;used=strlen(dst);if(used>=dst_size)return 0;available=dst_size-used-1;n=strlen(src);if(n>available)return 0;memcpy(dst+used,src,n+1);return 1;}
static int source_action(const char *request){return contains_ci(request,"scan")||contains_ci(request,"inspect")||contains_ci(request,"search source");}
static int action_request(const char *request){const char *p=request;char word[32];size_t n=0;if(request==NULL)return 0;while(*p&&isspace((unsigned char)*p))++p;while(*p&&isalpha((unsigned char)*p)&&n+1<sizeof(word))word[n++]=(char)tolower((unsigned char)*p++);word[n]='\0';return strcmp(word,"create")==0||strcmp(word,"build")==0||strcmp(word,"write")==0||strcmp(word,"generate")==0||strcmp(word,"make")==0||strcmp(word,"implement")==0||strcmp(word,"produce")==0;}

static int lesson_name(const char *request,char *name,size_t name_size)
{
    const char *p=request;size_t n=0,i;
    while(*p&&isspace((unsigned char)*p))++p;
    if(tolower((unsigned char)p[0])!='i'||tolower((unsigned char)p[1])!='n'||tolower((unsigned char)p[2])!='g'||tolower((unsigned char)p[3])!='e'||tolower((unsigned char)p[4])!='s'||tolower((unsigned char)p[5])!='t')return 0;
    p+=6;if(*p&&!isspace((unsigned char)*p))return 0;while(*p&&isspace((unsigned char)*p))++p;if(!*p)return 0;
    for(i=0;p[i]&&!isspace((unsigned char)p[i]);++i){unsigned char c=(unsigned char)p[i];if(!(isalnum(c)||c=='_'||c=='-')||n+1>=name_size)return 0;name[n++]=(char)tolower(c);}
    name[n]='\0';while(p[i]&&isspace((unsigned char)p[i]))++i;return p[i]=='\0'&&n>0;
}

static int lesson_action(const char *request,digit_dispatcher_result_t *out)
{
    digit_lesson_ingest_request_t in;digit_lesson_ingest_result_t result;char name[DIGIT_LESSON_NAME_MAX];size_t used=0;stnlabz_module_result_t sr;
    memset(name,0,sizeof(name));if(!lesson_name(request,name,sizeof(name)))return 0;
    memset(&in,0,sizeof(in));memset(&result,0,sizeof(result));snprintf(in.name,sizeof(in.name),"%s",name);
    sr=dispatcher_host->invoke_service(DIGIT_LESSON_INGEST_SERVICE,&in,sizeof(in),&result,sizeof(result),&used);
    out->answered=1;
    if(sr!=STNLABZ_MODULE_OK||used!=sizeof(result)){snprintf(out->answer,sizeof(out->answer),"I couldn't ingest lesson %s.",name);return 1;}
    snprintf(out->answer,sizeof(out->answer),"Lesson %s ingested. %zu units processed, %zu stored, %zu rejected.",name,result.units,result.stored,result.rejected);return 1;
}

static int resolve_project(const char *request,char *project,size_t project_size){digit_source_projects_result_t projects;size_t used=0,i;stnlabz_module_result_t sr;memset(&projects,0,sizeof(projects));sr=dispatcher_host->invoke_service(DIGIT_SOURCE_PROJECTS_SERVICE,NULL,0,&projects,sizeof(projects),&used);if(sr!=STNLABZ_MODULE_OK||used!=sizeof(projects))return 0;for(i=0;i<projects.count&&i<DIGIT_SOURCE_PROJECTS_MAX;++i)if(contains_ci(request,projects.projects[i].name)){snprintf(project,project_size,"%s",projects.projects[i].name);return 1;}return 0;}

static void source_scan(const char *request,digit_dispatcher_result_t *out){char project[DIGIT_SOURCE_PROJECT_MAX];digit_source_status_request_t status_request;digit_source_status_result_t status;digit_source_search_request_t search_request;digit_source_search_result_t found;size_t used=0,i,offset=0;stnlabz_module_result_t sr;memset(project,0,sizeof(project));if(!resolve_project(request,project,sizeof(project))){out->answered=1;snprintf(out->answer,sizeof(out->answer),"I can't identify that source project. Ask me for the available source projects and I can tell you what I can inspect.");return;}memset(&status_request,0,sizeof(status_request));memset(&status,0,sizeof(status));snprintf(status_request.project,sizeof(status_request.project),"%s",project);sr=dispatcher_host->invoke_service(DIGIT_SOURCE_STATUS_SERVICE,&status_request,sizeof(status_request),&status,sizeof(status),&used);if(sr!=STNLABZ_MODULE_OK||used!=sizeof(status)){out->answered=1;snprintf(out->answer,sizeof(out->answer),"I found source project %s, but I couldn't inspect its status.",project);return;}offset=(size_t)snprintf(out->answer,sizeof(out->answer),"I scanned source project %s. Git repository: %s",project,status.git_repository?"yes":"no");if(status.git_repository&&status.head[0]!='\0'&&offset<sizeof(out->answer))offset+=(size_t)snprintf(out->answer+offset,sizeof(out->answer)-offset,". HEAD: %s",status.head);memset(&search_request,0,sizeof(search_request));memset(&found,0,sizeof(found));used=0;snprintf(search_request.project,sizeof(search_request.project),"%s",project);snprintf(search_request.query,sizeof(search_request.query),"module");sr=dispatcher_host->invoke_service(DIGIT_SOURCE_SEARCH_SERVICE,&search_request,sizeof(search_request),&found,sizeof(found),&used);if(sr==STNLABZ_MODULE_OK&&used==sizeof(found)&&offset<sizeof(out->answer)){int w=snprintf(out->answer+offset,sizeof(out->answer)-offset,". I found %zu bounded source matches",found.count);if(w>0&&(size_t)w<sizeof(out->answer)-offset)offset+=(size_t)w;for(i=0;i<found.count&&i<3&&offset<sizeof(out->answer);++i){w=snprintf(out->answer+offset,sizeof(out->answer)-offset,"; %s:%zu",found.matches[i].path,found.matches[i].line);if(w<=0||(size_t)w>=sizeof(out->answer)-offset)break;offset+=(size_t)w;}}if(offset<sizeof(out->answer)-1)snprintf(out->answer+offset,sizeof(out->answer)-offset,".");out->answered=1;}

static void dispatch_intent_response(const char *request,const digit_intent_result_t *intent,digit_dispatcher_result_t *out)
{
    digit_response_request_t rr;digit_response_result_t ro;size_t used=0;stnlabz_module_result_t sr;const char *intent_name;const char *target_name;
    if(intent==NULL){out->answered=1;snprintf(out->answer,sizeof(out->answer),"I couldn't interpret that request.");return;}
    intent_name=intent_class_name(intent->intent);target_name=intent_target_name(intent->target);
    memset(&rr,0,sizeof(rr));memset(&ro,0,sizeof(ro));
    if(snprintf(rr.question,sizeof(rr.question),"INTENT: %s\nTARGET: %s\nSUBJECT: %s\nREQUEST: %s",intent_name,target_name,intent->subject,request)>=(int)sizeof(rr.question)){out->answered=1;snprintf(out->answer,sizeof(out->answer),"The interpreted request is too large to dispatch safely.");return;}
    sr=dispatcher_host->invoke_service(DIGIT_RESPONSE_SERVICE,&rr,sizeof(rr),&ro,sizeof(ro),&used);
    if(sr!=STNLABZ_MODULE_OK||used!=sizeof(ro)){out->answered=1;snprintf(out->answer,sizeof(out->answer),"I couldn't complete that request because one of my internal services is unavailable.");return;}
    out->answered=1;snprintf(out->answer,sizeof(out->answer),"%s",ro.answer[0]?ro.answer:"I couldn't produce an answer for that request.");
}

/* [AI:GPT-6 | 2026-10-09] No generator/executor is registered in the
 * Dispatcher action path. Never repackage an action as evidence retrieval. */
static void unsupported_action(digit_dispatcher_result_t *out){
 out->answered=1;
 snprintf(out->answer,sizeof(out->answer),
  "I cannot execute or generate that requested action with my currently registered Dispatcher capabilities.");
}

static stnlabz_module_result_t dispatcher_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *handler_context)
{
    const digit_dispatcher_request_t *in=request;
    digit_dispatcher_result_t out;
    digit_interpretation_request_t interpretation_request;
    digit_interpretation_result_t interpretation_result;
    digit_intent_request_t intent_request;
    digit_intent_result_t intent_result;
    const char *interpreted_request;
    size_t interpretation_used=0,intent_used=0;
    stnlabz_module_result_t interpretation_sr,intent_sr;
    (void)handler_context;
    if(request==NULL||request_size!=sizeof(*in)||response==NULL||response_used==NULL||response_size<sizeof(out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(memchr(in->request,'\0',sizeof(in->request))==NULL||in->request[0]=='\0')return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(&out,0,sizeof(out));
    memset(&interpretation_request,0,sizeof(interpretation_request));
    memset(&interpretation_result,0,sizeof(interpretation_result));
    snprintf(interpretation_request.text,sizeof(interpretation_request.text),"%s",in->request);
    interpretation_sr=dispatcher_host->invoke_service(DIGIT_INTERPRETATION_SERVICE,&interpretation_request,sizeof(interpretation_request),&interpretation_result,sizeof(interpretation_result),&interpretation_used);
    if(interpretation_sr!=STNLABZ_MODULE_OK||interpretation_used!=sizeof(interpretation_result)||!interpretation_result.resolved){
        out.answered=1;
        snprintf(out.answer,sizeof(out.answer),"I couldn't interpret that request because my Interpretation service is unavailable.");
        normalize_answer(out.answer);
        memcpy(response,&out,sizeof(out));*response_used=sizeof(out);return STNLABZ_MODULE_OK;
    }
    if(memchr(interpretation_result.normalized,'\0',sizeof(interpretation_result.normalized))==NULL||
       interpretation_result.normalized[0]=='\0'){
        out.answered=1;
        snprintf(out.answer,sizeof(out.answer),"Interpretation returned an invalid request.");
        memcpy(response,&out,sizeof(out));*response_used=sizeof(out);return STNLABZ_MODULE_OK;
    }
    interpreted_request=interpretation_result.normalized;
    memset(&intent_request,0,sizeof(intent_request));memset(&intent_result,0,sizeof(intent_result));
    snprintf(intent_request.text,sizeof(intent_request.text),"%s",interpreted_request);
    intent_sr=dispatcher_host->invoke_service(DIGIT_INTENT_SERVICE,&intent_request,sizeof(intent_request),&intent_result,sizeof(intent_result),&intent_used);
    if(intent_sr!=STNLABZ_MODULE_OK||intent_used!=sizeof(intent_result)){
        char diagnostic[256];
        out.answered=1;
        if(intent_sr!=STNLABZ_MODULE_OK)
            snprintf(out.answer,sizeof(out.answer),"Intent service invocation failed (status %d).",(int)intent_sr);
        else
            snprintf(out.answer,sizeof(out.answer),"Intent service returned an invalid response size (%zu; expected %zu).",intent_used,sizeof(intent_result));
        snprintf(diagnostic,sizeof(diagnostic),"[DISPATCHER] intent.interpret failure: status=%d returned=%zu expected=%zu",(int)intent_sr,intent_used,sizeof(intent_result));
        if(dispatcher_host->send_message!=NULL)(void)dispatcher_host->send_message(diagnostic);
    }
    /* [AI:GPT-6 | 2026-10-09] Only explicitly named, supported
     * capabilities execute. A generic ACTION cannot enter retrieval. */
    /* [AI:GPT-6 | 2026-10-09] Require an established, target-consistent
     * Intent before executing any capability, including lesson ingestion. */
    else if(!intent_result.established ||
            (intent_result.intent==DIGIT_INTENT_ACTION &&
             intent_result.target!=DIGIT_INTENT_TARGET_CAPABILITY)){
        out.answered=1;
        snprintf(out.answer,sizeof(out.answer),"I can't establish an authorized request to dispatch.");
    }
    else if(intent_result.intent==DIGIT_INTENT_ACTION){
        if(lesson_action(interpreted_request,&out)){}
        else if(source_action(interpreted_request))source_scan(interpreted_request,&out);
        else unsupported_action(&out);
    }
    else if(intent_result.intent==DIGIT_INTENT_STATUS&&source_action(interpreted_request))source_scan(interpreted_request,&out);
    else if(intent_result.intent==DIGIT_INTENT_UNKNOWN||intent_result.intent==DIGIT_INTENT_AMBIGUOUS){out.answered=1;snprintf(out.answer,sizeof(out.answer),"I can't establish what you want me to do from that request.");}
    else dispatch_intent_response(interpreted_request,&intent_result,&out);
    normalize_answer(out.answer);
    memcpy(response,&out,sizeof(out));*response_used=sizeof(out);return STNLABZ_MODULE_OK;
}

/* [AI:GPT-6 | 2026-10-09] Executed, deterministic local qualification;
 * service integration and runtime acceptance remain separate gates. */
static stnlabz_module_result_t dispatcher_qualify(stnlabz_module_qualification_result_t *result){
 char name[DIGIT_LESSON_NAME_MAX];digit_dispatcher_result_t out;
 size_t passed=0,negative_passed=0;
 if(result==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 memset(result,0,sizeof(*result));
 passed+=(size_t)action_request("Create a module");
 passed+=(size_t)action_request("write code");
 passed+=(size_t)action_request("  Generate a report");
 passed+=(size_t)!action_request("what is the mission?");
 passed+=(size_t)!action_request("compare C and Python");
 passed+=(size_t)source_action("inspect source");
 passed+=(size_t)!source_action("who are you?");
 passed+=(size_t)lesson_name("ingest unit-one",name,sizeof(name))&&strcmp(name,"unit-one")==0;
 passed+=(size_t)!lesson_name("ingest unit-one extra",name,sizeof(name));
 memset(&out,0,sizeof(out));unsupported_action(&out);
 passed+=(size_t)(out.answered==1&&strstr(out.answer,"cannot execute or generate")!=NULL);
 negative_passed+=(size_t)!lesson_name("ingest ../unsafe",name,sizeof(name));
 result->tests_executed=10;result->tests_passed=passed;
 result->tests_failed=result->tests_executed-result->tests_passed;
 result->negative_test_executed=1;result->negative_test_passed=negative_passed;
 return result->tests_failed||result->negative_test_passed!=result->negative_test_executed?STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t dispatcher_start(const stnlabz_module_host_t *host){if(host==NULL||host->register_service==NULL||host->invoke_service==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;if(!host->register_service(DIGIT_DISPATCHER_SERVICE,dispatcher_service,NULL))return STNLABZ_MODULE_ERR_START_FAILED;dispatcher_host=host;if(host->send_message!=NULL)(void)host->send_message("[DISPATCHER] active: operator requests coordinated across Digit services including lesson ingestion");return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t dispatcher_stop(void){if(dispatcher_host!=NULL&&dispatcher_host->unregister_service!=NULL)if(!dispatcher_host->unregister_service(DIGIT_DISPATCHER_SERVICE,NULL))return STNLABZ_MODULE_ERR_STOP_FAILED;dispatcher_host=NULL;return STNLABZ_MODULE_OK;}
static const stnlabz_module_descriptor_t dispatcher_descriptor={"dispatcher","Digit Dispatcher",1,3,1,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,dispatcher_qualify,dispatcher_start,dispatcher_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &dispatcher_descriptor;}
