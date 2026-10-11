#include <string.h>
#include "chatgpt.h"
#include "chatgpt_oauth.h"
/* [AI:GPT-6 | 2026-10-11] In-process ISO C integration module.
 * Learning requires authenticated identity AND explicit write authority.
 * This read-only module grants neither. Never forward text to operational
 * Dispatcher until a service-enforced non-mutating scope is qualified. */
static const stnlabz_module_host_t *active_host;
static stnlabz_module_result_t capabilities(const void *request,size_t request_size,
    void *response,size_t response_size,size_t *used,void *ctx){
    const digit_chatgpt_capabilities_t result={1U,0U,0U,0U};
    (void)ctx;
    if(request||request_size||!response||!used||response_size<sizeof(result))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memcpy(response,&result,sizeof(result));*used=sizeof(result);return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t test_case(const void *request,size_t request_size,
    void *response,size_t response_size,size_t *used,void *ctx){
    (void)request;(void)request_size;(void)response;(void)response_size;(void)ctx;
    if(used)*used=0;
    return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
}
/* [AI:GPT-6 | 2026-10-11] Internal MCP JSON-RPC adapter; authorization and
 * HTTP transport belong to Interface, not to a public socket here.
 * No live Digit operation is exposed until an authenticated binding exists. */
static stnlabz_module_result_t mcp_request(const void *request,size_t request_size,
    void *response,size_t response_size,size_t *used,void *ctx){
    const digit_chatgpt_mcp_request_t *in=request;
    digit_chatgpt_mcp_result_t *out=response;
    const char *reply;
    size_t n;
    (void)ctx;
    if(used)*used=0;
    if(!in||request_size!=sizeof(*in)||!out||
       response_size<sizeof(*out)||!used||
       !memchr(in->json,0,sizeof(in->json)))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    /* Only exact bounded, notification-free protocol probes are admitted.
     * A proper JSON-RPC parser/validated request IDs are needed before tools/call. */
    if(strcmp(in->json,"{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}")==0)
       reply="{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}";
    else if(strcmp(in->json,"{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}")==0)
       reply="{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"tools\":[]}}";
    else return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    n=strlen(reply);
    if(n>=sizeof(out->json))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(out,0,sizeof(*out));
    memcpy(out->json,reply,n+1);
    *used=sizeof(*out);
    return STNLABZ_MODULE_OK;
}
/* In-process OAuth route service; public HTTP routing is a separate binding. */
static stnlabz_module_result_t oauth_request(const void *request,size_t request_size,
    void *response,size_t response_size,size_t *used,void *ctx){
    (void)ctx;
    if(used)*used=0;
    if(!request||request_size!=sizeof(digit_chatgpt_oauth_request_t)||
       !response||response_size<sizeof(digit_chatgpt_oauth_response_t)||!used)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!digit_chatgpt_oauth_route(request,response))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    *used=sizeof(digit_chatgpt_oauth_response_t);
    return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t qualify(stnlabz_module_qualification_result_t *q){
    digit_chatgpt_capabilities_t result;
    digit_chatgpt_test_request_t request={{0}};
    digit_chatgpt_test_result_t reply={0};
    size_t used=0;unsigned int passed=0;
    if(!q)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(q,0,sizeof(*q));
    passed+=(unsigned)(capabilities(NULL,0,&result,sizeof(result),&used,NULL)==STNLABZ_MODULE_OK&&
        used==sizeof(result)&&result.version==1&&result.interface_bound==0&&
        result.non_mutating_qualified==0&&result.conversations_enabled==0);
    passed+=(unsigned)(capabilities("x",1,&result,sizeof(result),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
    passed+=(unsigned)(capabilities(NULL,0,&result,sizeof(result)-1,&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
    passed+=(unsigned)(capabilities(NULL,0,&result,sizeof(result),NULL,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
    passed+=(unsigned)(test_case(&request,sizeof(request),&reply,sizeof(reply),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT&&used==0);
    memset(&request,'X',sizeof(request));
    passed+=(unsigned)(test_case(&request,sizeof(request),&reply,sizeof(reply),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT&&used==0);
    passed+=(unsigned)(test_case(NULL,0,NULL,0,&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT&&used==0);
    /* Enforce the external ABI minimum with actual additional rejection
     * checks; no fabricated test counts or relaxed qualification gates. */
    passed+=(unsigned)(capabilities(NULL,0,NULL,sizeof(result),&used,NULL)==
        STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
    used=37;
    passed+=(unsigned)(test_case(&request,sizeof(request),&reply,sizeof(reply),
        NULL,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
    used=37;
    passed+=(unsigned)(test_case(&request,sizeof(request),&reply,sizeof(reply),
        &used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT&&used==0);
    {digit_chatgpt_mcp_request_t in={{0}};
     digit_chatgpt_mcp_result_t out={{0}};
     strcpy(in.json,"{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
     passed+=(unsigned)(mcp_request(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_OK&&strstr(out.json,"\"result\"")!=NULL);
     strcpy(in.json,"{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
     passed+=(unsigned)(mcp_request(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_OK&&strstr(out.json,"\"tools\":[]")!=NULL);
     strcpy(in.json,"{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\"}");
     passed+=(unsigned)(mcp_request(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
     memset(&in,'X',sizeof(in));
     passed+=(unsigned)(mcp_request(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT);
    }
    q->tests_executed=14;q->tests_passed=passed;q->tests_failed=14-passed;
    q->negative_test_executed=11;q->negative_test_passed=q->tests_failed==0?11:0;
    return q->tests_failed==0?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_START_FAILED;
}
static stnlabz_module_result_t start(const stnlabz_module_host_t *host){
    if(!host||!host->register_service||!host->unregister_service)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!host->register_service(DIGIT_CHATGPT_CAPABILITIES_SERVICE,capabilities,NULL))
        return STNLABZ_MODULE_ERR_START_FAILED;
    if(!host->register_service(DIGIT_CHATGPT_TEST_SERVICE,test_case,NULL)){
        (void)host->unregister_service(DIGIT_CHATGPT_CAPABILITIES_SERVICE,NULL);
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    if(!host->register_service(DIGIT_CHATGPT_MCP_SERVICE,mcp_request,NULL)){
        (void)host->unregister_service(DIGIT_CHATGPT_TEST_SERVICE,NULL);
        (void)host->unregister_service(DIGIT_CHATGPT_CAPABILITIES_SERVICE,NULL);
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    if(!host->register_service(DIGIT_CHATGPT_OAUTH_SERVICE,oauth_request,NULL)){
        (void)host->unregister_service(DIGIT_CHATGPT_MCP_SERVICE,NULL);
        (void)host->unregister_service(DIGIT_CHATGPT_TEST_SERVICE,NULL);
        (void)host->unregister_service(DIGIT_CHATGPT_CAPABILITIES_SERVICE,NULL);
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    active_host=host;
    if(host->send_message)(void)host->send_message("[CHATGPT] integration registered; execution denied until qualified authenticated test scope");
    return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t stop(void){
    int ok=1;
    if(active_host&&active_host->unregister_service){
        if(!active_host->unregister_service(DIGIT_CHATGPT_OAUTH_SERVICE,NULL))ok=0;
        if(!active_host->unregister_service(DIGIT_CHATGPT_MCP_SERVICE,NULL))ok=0;
        if(!active_host->unregister_service(DIGIT_CHATGPT_TEST_SERVICE,NULL))ok=0;
        if(!active_host->unregister_service(DIGIT_CHATGPT_CAPABILITIES_SERVICE,NULL))ok=0;
    }
    active_host=NULL;return ok?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_STOP_FAILED;
}
static const stnlabz_module_descriptor_t descriptor={"chatgpt","Digit ChatGPT Integration",1,0,6,
    STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,qualify,start,stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &descriptor;}
