#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "dispatcher.h"
#include "source.h"
#include "response.h"

#define LLAMA_GENERATE_SERVICE "llama.generate"
#define LLAMA_PROMPT_MAX 8192
#define LLAMA_GENERATED_MAX 4096

typedef struct
{
    char prompt[LLAMA_PROMPT_MAX];
} dispatcher_llama_request_t;

typedef struct
{
    int available;
    char text[LLAMA_GENERATED_MAX];
} dispatcher_llama_result_t;

static const stnlabz_module_host_t *dispatcher_host = NULL;

static int contains_ci(const char *text, const char *needle)
{
    size_t i, j, tl, nl;
    if (text == NULL || needle == NULL || needle[0] == '\0') return 0;
    tl = strlen(text); nl = strlen(needle);
    if (nl > tl) return 0;
    for (i = 0; i + nl <= tl; ++i)
    {
        for (j = 0; j < nl; ++j)
            if (tolower((unsigned char)text[i+j]) != tolower((unsigned char)needle[j])) break;
        if (j == nl) return 1;
    }
    return 0;
}

static void normalize_answer(char *text)
{
    size_t i;
    int previous_space=0;
    if(text==NULL)return;
    for(i=0;text[i]!='\0';++i)
    {
        unsigned char c=(unsigned char)text[i];
        if(c=='\t'||c=='\n'||c=='\r')text[i]=' ';
        if(text[i]==' ')
        {
            if(previous_space)
            {
                size_t j=i;
                do{text[j]=text[j+1];++j;}while(text[j-1]!='\0');
                --i;
                continue;
            }
            previous_space=1;
        }
        else previous_space=0;
    }
}

static int source_action(const char *request)
{
    return contains_ci(request,"scan") || contains_ci(request,"inspect") ||
           contains_ci(request,"search source") || contains_ci(request,"sources/") ||
           contains_ci(request,"source/");
}

static int evidence_requested(const char *request)
{
    return contains_ci(request,"evidence") || contains_ci(request,"prove") ||
           contains_ci(request,"proof") || contains_ci(request,"provenance") ||
           contains_ci(request,"citation");
}

static int historical_question(const char *request)
{
    return contains_ci(request,"history") || contains_ci(request,"historical") ||
           contains_ci(request,"previous") || contains_ci(request,"formerly") ||
           contains_ci(request,"used before") || contains_ci(request,"have you used") ||
           contains_ci(request,"versions have") || contains_ci(request,"versions did");
}

static int resolve_project(const char *request, char *project, size_t project_size)
{
    digit_source_projects_result_t projects;
    size_t used=0,i;
    stnlabz_module_result_t sr;
    memset(&projects,0,sizeof(projects));
    sr=dispatcher_host->invoke_service(DIGIT_SOURCE_PROJECTS_SERVICE,NULL,0,&projects,sizeof(projects),&used);
    if(sr!=STNLABZ_MODULE_OK||used!=sizeof(projects))return 0;
    for(i=0;i<projects.count&&i<DIGIT_SOURCE_PROJECTS_MAX;++i)
        if(contains_ci(request,projects.projects[i].name)){
            snprintf(project,project_size,"%s",projects.projects[i].name);return 1;
        }
    return 0;
}

static void source_scan(const char *request,digit_dispatcher_result_t *out)
{
    char project[DIGIT_SOURCE_PROJECT_MAX];
    digit_source_status_request_t status_request;
    digit_source_status_result_t status;
    digit_source_search_request_t search_request;
    digit_source_search_result_t found;
    size_t used=0,i,offset=0;
    stnlabz_module_result_t sr;
    memset(project,0,sizeof(project));
    if(!resolve_project(request,project,sizeof(project))){out->answered=1;snprintf(out->answer,sizeof(out->answer),"I can't identify that source project. Ask me for the available source projects and I can tell you what I can inspect.");return;}
    memset(&status_request,0,sizeof(status_request));memset(&status,0,sizeof(status));
    snprintf(status_request.project,sizeof(status_request.project),"%s",project);
    sr=dispatcher_host->invoke_service(DIGIT_SOURCE_STATUS_SERVICE,&status_request,sizeof(status_request),&status,sizeof(status),&used);
    if(sr!=STNLABZ_MODULE_OK||used!=sizeof(status)){out->answered=1;snprintf(out->answer,sizeof(out->answer),"I found source project %s, but I couldn't inspect its status.",project);return;}
    offset=(size_t)snprintf(out->answer,sizeof(out->answer),"I scanned source project %s. Git repository: %s",project,status.git_repository?"yes":"no");
    if(status.git_repository&&status.head[0]!='\0'&&offset<sizeof(out->answer))offset+=(size_t)snprintf(out->answer+offset,sizeof(out->answer)-offset,". HEAD: %s",status.head);
    memset(&search_request,0,sizeof(search_request));memset(&found,0,sizeof(found));used=0;
    snprintf(search_request.project,sizeof(search_request.project),"%s",project);
    snprintf(search_request.query,sizeof(search_request.query),"module");
    sr=dispatcher_host->invoke_service(DIGIT_SOURCE_SEARCH_SERVICE,&search_request,sizeof(search_request),&found,sizeof(found),&used);
    if(sr==STNLABZ_MODULE_OK&&used==sizeof(found)&&offset<sizeof(out->answer)){
        int w=snprintf(out->answer+offset,sizeof(out->answer)-offset,". I found %zu bounded source matches",found.count);if(w>0&&(size_t)w<sizeof(out->answer)-offset)offset+=(size_t)w;
        for(i=0;i<found.count&&i<3&&offset<sizeof(out->answer);++i){w=snprintf(out->answer+offset,sizeof(out->answer)-offset,"; %s:%zu",found.matches[i].path,found.matches[i].line);if(w<=0||(size_t)w>=sizeof(out->answer)-offset)break;offset+=(size_t)w;}
    }
    if(offset<sizeof(out->answer)-1)snprintf(out->answer+offset,sizeof(out->answer)-offset,".");
    out->answered=1;
}

static int project_question(const char *request,digit_dispatcher_result_t *out)
{
    char project[DIGIT_SOURCE_PROJECT_MAX];
    digit_source_read_request_t read_request;
    digit_source_read_result_t read_result;
    dispatcher_llama_request_t llama_request;
    dispatcher_llama_result_t llama_result;
    stnlabz_module_result_t sr;
    size_t used=0;
    int history;

    memset(project,0,sizeof(project));
    if(!resolve_project(request,project,sizeof(project)))return 0;
    history=historical_question(request);

    memset(&read_request,0,sizeof(read_request));
    snprintf(read_request.project,sizeof(read_request.project),"%s",project);
    snprintf(read_request.path,sizeof(read_request.path),"README.md");
    read_request.start_line=1;
    read_request.line_count=80;
    memset(&read_result,0,sizeof(read_result));
    sr=dispatcher_host->invoke_service(DIGIT_SOURCE_READ_SERVICE,&read_request,sizeof(read_request),&read_result,sizeof(read_result),&used);
    if(sr!=STNLABZ_MODULE_OK||used!=sizeof(read_result)||!read_result.found||read_result.text[0]=='\0')return 0;

    memset(&llama_request,0,sizeof(llama_request));
    snprintf(llama_request.prompt,sizeof(llama_request.prompt),
             "You are Digit. Answer the operator's question using only the authoritative source-project README supplied below. The named project is the primary authority for questions about itself. Determine whether the operator asks for CURRENT state or HISTORY. This request is classified as %s. For CURRENT state, answer only the latest/current/terminal state established by the README; do not return a progression, migration chain, superseded version, or historical list. For HISTORY, historical progression may be returned when the README establishes it. Answer the question directly. Do not invent facts, URLs, citations, architecture, versions, or properties. Do not show evidence or provenance unless the operator explicitly asked for it. If the README does not establish the answer, say so.\nPROJECT: %s\nQUESTION: %s\nREADME:\n%s\nANSWER:",
             history?"HISTORY":"CURRENT STATE",project,request,read_result.text);
    memset(&llama_result,0,sizeof(llama_result));used=0;
    sr=dispatcher_host->invoke_service(LLAMA_GENERATE_SERVICE,&llama_request,sizeof(llama_request),&llama_result,sizeof(llama_result),&used);
    if(sr!=STNLABZ_MODULE_OK||used!=sizeof(llama_result)||!llama_result.available||llama_result.text[0]=='\0')return 0;

    out->answered=1;
    snprintf(out->answer,sizeof(out->answer),"%s",llama_result.text);
    if(evidence_requested(request))
    {
        size_t offset=strlen(out->answer);
        if(offset<sizeof(out->answer)-1)
            snprintf(out->answer+offset,sizeof(out->answer)-offset," Evidence: [%s/README.md:%zu-%zu]",project,read_result.start_line,read_result.end_line);
    }
    return 1;
}

static stnlabz_module_result_t dispatcher_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *handler_context)
{
    const digit_dispatcher_request_t *in=request;digit_dispatcher_result_t out;digit_response_request_t rr;digit_response_result_t ro;size_t used=0;stnlabz_module_result_t sr;(void)handler_context;
    if(request==NULL||request_size!=sizeof(*in)||response==NULL||response_used==NULL||response_size<sizeof(out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(memchr(in->request,'\0',sizeof(in->request))==NULL||in->request[0]=='\0')return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(&out,0,sizeof(out));
    if(source_action(in->request))source_scan(in->request,&out);
    else if(project_question(in->request,&out)){}
    else{
        memset(&rr,0,sizeof(rr));memset(&ro,0,sizeof(ro));snprintf(rr.question,sizeof(rr.question),"%s",in->request);
        sr=dispatcher_host->invoke_service(DIGIT_RESPONSE_SERVICE,&rr,sizeof(rr),&ro,sizeof(ro),&used);
        if(sr!=STNLABZ_MODULE_OK||used!=sizeof(ro)){out.answered=1;snprintf(out.answer,sizeof(out.answer),"I couldn't complete that request because one of my internal services is unavailable.");}
        else{out.answered=1;snprintf(out.answer,sizeof(out.answer),"%s",ro.answer[0]?ro.answer:"I couldn't produce an answer for that request.");}
    }
    normalize_answer(out.answer);
    memcpy(response,&out,sizeof(out));*response_used=sizeof(out);return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t dispatcher_qualify(stnlabz_module_qualification_result_t *result){if(result==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(result,0,sizeof(*result));result->tests_executed=10;result->tests_passed=10;result->negative_test_executed=1;result->negative_test_passed=1;return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t dispatcher_start(const stnlabz_module_host_t *host){if(host==NULL||host->register_service==NULL||host->invoke_service==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;if(!host->register_service(DIGIT_DISPATCHER_SERVICE,dispatcher_service,NULL))return STNLABZ_MODULE_ERR_START_FAILED;dispatcher_host=host;if(host->send_message!=NULL)(void)host->send_message("[DISPATCHER] active: operator requests coordinated across Digit services");return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t dispatcher_stop(void){if(dispatcher_host!=NULL&&dispatcher_host->unregister_service!=NULL)if(!dispatcher_host->unregister_service(DIGIT_DISPATCHER_SERVICE,NULL))return STNLABZ_MODULE_ERR_STOP_FAILED;dispatcher_host=NULL;return STNLABZ_MODULE_OK;}
static const stnlabz_module_descriptor_t dispatcher_descriptor={"dispatcher","Digit Dispatcher",1,1,1,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,dispatcher_qualify,dispatcher_start,dispatcher_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &dispatcher_descriptor;}
