#include <stdio.h>
#include <string.h>

#include "response.h"

static unsigned int executed = 0;
static unsigned int failed = 0;

static void check(int condition, const char *name)
{
    ++executed;
    if (condition) printf("PASS %02u - %s\n", executed, name);
    else { ++failed; printf("FAIL %02u - %s\n", executed, name); }
}

/* [AI:GPT-6 | 2026-10-09] Exercise the real response service using
 * deterministic mock Validator input; prevent fabricated math evidence. */
static stnlabz_module_service_handler_fn response_handler;
static unsigned int unexpected_services;
typedef struct {char raw[4096];} inbound_request_t;
typedef struct {int status;int confidence;char normalized[4096];char reason[256];} inbound_result_t;
static int register_response(const char *name,stnlabz_module_service_handler_fn fn,void *ctx){
 (void)ctx;
 if(strcmp(name,DIGIT_RESPONSE_SERVICE))return 0;
 response_handler=fn;return 1;
}
static int unregister_response(const char *name,void *ctx){
 (void)ctx;return !strcmp(name,DIGIT_RESPONSE_SERVICE);
}
static stnlabz_module_result_t test_invoke(const char *name,const void *request,size_t request_size,
 void *response,size_t response_size,size_t *used){
 if(!strcmp(name,"validator.inbound")){
  const inbound_request_t *in=request;inbound_result_t *out=response;
  if(request_size!=sizeof(*in)||response_size<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  memset(out,0,sizeof(*out));
  snprintf(out->normalized,sizeof(out->normalized),"%s",in->raw);
  *used=sizeof(*out);return STNLABZ_MODULE_OK;
 }
 unexpected_services++;
 return STNLABZ_MODULE_ERR_NOT_FOUND;
}
static void check_arithmetic_service(const stnlabz_module_descriptor_t *descriptor){
 stnlabz_module_host_t host={0};
 digit_response_request_t request={0};
 digit_response_result_t answer={0};
 size_t used=0;
 host.register_service=register_response;host.unregister_service=unregister_response;
 host.invoke_service=test_invoke;
 check(descriptor->start(&host)==STNLABZ_MODULE_OK&&response_handler!=NULL,
       "response service registers with test host");
 snprintf(request.question,sizeof(request.question),
  "INTENT: FACT\\nTARGET: KNOWLEDGE\\nSUBJECT: \\nREQUEST: What is 2 plus 2?");
 unexpected_services=0;
 check(response_handler&&response_handler(&request,sizeof(request),&answer,
       sizeof(answer),&used,NULL)==STNLABZ_MODULE_OK&&used==sizeof(answer)&&
       answer.answered&&strstr(answer.answer,"cannot verify that calculation")!=NULL,
       "arithmetic question cannot return unrelated evidence");
 check(unexpected_services==0,"unsupported calculation does not invoke Corpus or Reasoning");
 check(descriptor->stop()==STNLABZ_MODULE_OK,"response service unregisters");
}

int main(void)
{
    const stnlabz_module_descriptor_t *descriptor = stnlabz_module_get_descriptor();
    stnlabz_module_qualification_result_t qualification;
    digit_response_request_t request;
    digit_response_result_t result;

    memset(&request, 0, sizeof(request));
    memset(&result, 0, sizeof(result));
    check(descriptor != NULL, "descriptor is exported");
    check(descriptor != NULL && strcmp(descriptor->id, "response") == 0, "module identity is response");
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 6 && descriptor->version_patch == 10, "internal version is 1.6.10");
    check(descriptor != NULL && descriptor->qualify != NULL, "qualification callback exists");
    check(descriptor != NULL && descriptor->qualify(&qualification) == STNLABZ_MODULE_OK, "qualification executes");
    check(qualification.tests_executed >= STNLABZ_MODULE_MIN_TESTS, "required test count is reported");
    check(qualification.tests_passed == qualification.tests_executed && qualification.tests_failed == 0, "required tests pass");
    check(qualification.negative_test_executed && qualification.negative_test_passed, "negative validation passes");
    check(sizeof(request.question) == DIGIT_RESPONSE_QUESTION_MAX, "question contract is bounded");
    check(sizeof(result.answer) == DIGIT_RESPONSE_ANSWER_MAX, "answer contract is bounded");
    check_arithmetic_service(descriptor);
    printf("\nResponse module tests: %u executed, %u failed\n", executed, failed);
    return failed == 0 ? 0 : 1;
}
