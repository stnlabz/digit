#include <stdio.h>
#include <string.h>

#include "response.h"
#include "arithmetic.h"

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
static unsigned int corpus_queries;
static unsigned int outbound_validations;
static unsigned int arithmetic_calls;
static const char *arithmetic_expected_request;
static const char *arithmetic_expected_answer;
static int arithmetic_available=1;
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
 if(!strcmp(name,DIGIT_ARITHMETIC_SERVICE)){
  const digit_arithmetic_request_t *in=request;
  digit_arithmetic_result_t *out=response;
  long long left,right,value,remainder;
  char op,tail;
  if(request_size!=sizeof(*in)||response_size<sizeof(*out)||
     !arithmetic_expected_request||strcmp(in->expression,arithmetic_expected_request))
   return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  arithmetic_calls++;
  if(!arithmetic_available)return STNLABZ_MODULE_ERR_NOT_FOUND;
  memset(out,0,sizeof(*out));
  if((strchr(arithmetic_expected_request,'.') || strchr(arithmetic_expected_request,'=') || strchr(arithmetic_expected_request,'(')) &&
     arithmetic_expected_answer[0] &&
     strstr(arithmetic_expected_answer," = ")){
   out->status=DIGIT_ARITHMETIC_OK;
   snprintf(out->decimal_answer,sizeof(out->decimal_answer),"%s",arithmetic_expected_answer);
  }else if(!strcmp(arithmetic_expected_answer,"Division by zero is undefined."))
   out->status=DIGIT_ARITHMETIC_DIVIDE_BY_ZERO;
  else if(sscanf(arithmetic_expected_answer,"%lld %c %lld = %lld remainder %lld%c",
        &left,&op,&right,&value,&remainder,&tail)>=5){
   out->status=DIGIT_ARITHMETIC_OK;out->left=left;out->right=right;
   out->value=value;out->remainder=remainder;out->operation=op;
  }else if(sscanf(arithmetic_expected_answer,"%lld %c %lld = %lld.%c",
        &left,&op,&right,&value,&tail)==4){
   out->status=DIGIT_ARITHMETIC_OK;out->left=left;out->right=right;
   out->value=value;out->operation=op;
  }else out->status=DIGIT_ARITHMETIC_INVALID;
  *used=sizeof(*out);
  return STNLABZ_MODULE_OK;
 }
 if(!strcmp(name,"corpus.search")){
  corpus_queries++;
  memset(response,0,response_size);
  *used=response_size;
  return STNLABZ_MODULE_OK;
 }
 if(!strcmp(name,"validator.outbound")){
  outbound_validations++;
  return STNLABZ_MODULE_ERR_NOT_FOUND;
 }
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
  "INTENT: FACT\nTARGET: KNOWLEDGE\nSUBJECT: \nREQUEST: What is 2 plus 2?");
 unexpected_services=0;arithmetic_calls=0;
 arithmetic_expected_request="What is 2 plus 2?";arithmetic_expected_answer="2 + 2 = 4.";
 check(response_handler&&response_handler(&request,sizeof(request),&answer,
       sizeof(answer),&used,NULL)==STNLABZ_MODULE_OK&&used==sizeof(answer)&&
       answer.answered&&strcmp(answer.answer,"2 + 2 = 4.")==0,
       "arithmetic question computes an exact answer");
 check(unexpected_services==0&&arithmetic_calls==1,"arithmetic uses shared service without Corpus or Reasoning");
 /* [AI:GPT-6 | 2026-10-09] Arithmetic execution contract and refusal boundaries. */
 {
  static const struct {const char *question,*expected;} cases[]={
   {"Digit what is 2+2?","2 + 2 = 4."},
   {"Digit what is 2 X 2?","2 * 2 = 4."},
   {"Digit what is 2 x 2?","2 * 2 = 4."},
   {"Digit what is 2 × 2?","2 * 2 = 4."},
   {"Digit what is 2 multiplied by 2?","2 * 2 = 4."},
   {"Digit what is 2 multiply by 2?","2 * 2 = 4."},
   {"What is 2 X 2?","2 * 2 = 4."},
   {"Digit what is 2 plus 2?","2 + 2 = 4."},
   {"Digit what is two plus two?","2 + 2 = 4."},
   {"Digit, what is 2 plus 2?","2 + 2 = 4."},
   {"Digital what is 2 plus 2?","I cannot evaluate that arithmetic expression within the supported two-operand integer limits."},
   {"What is 7 minus 12?","7 - 12 = -5."},
   {"What is 3 times 5?","3 * 5 = 15."},
   {"What is 9 divided by 3?","9 / 3 = 3."},
   {"Digit what is 8 / 2?","8 / 2 = 4."},
   {"Digit what is 1000 / 0.5?","1000 / 0.5 = 2000."},
   {"Digit what is 2*x+3=11?","x = 4."},
   {"Digit what is (2 + 3) * 4?","Result = 20."},
   {"Digit what is 8 ÷ 2?","8 / 2 = 4."},
   {"Digit what is 8 divide by 2?","8 / 2 = 4."},
   {"Digit what is 8 over 2?","8 / 2 = 4."},
   {"Digit what is 8 divided by zero?","Division by zero is undefined."},
   {"What is 7 divided by 2?","7 / 2 = 3 remainder 1."},
   {"What is 7 divided by 0?","Division by zero is undefined."},
   {"What is 1000000000 times 1000000000?","1000000000 * 1000000000 = 1000000000000000000."},
   {"What is 1000000001 plus 2?","I cannot evaluate that arithmetic expression within the supported two-operand integer limits."},
   {"What is 2 plus 2 plus 2?","I cannot evaluate that arithmetic expression within the supported two-operand integer limits."}
  };
  size_t i;
  for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
   memset(&request,0,sizeof(request));memset(&answer,0,sizeof(answer));used=0;
   snprintf(request.question,sizeof(request.question),"%s",cases[i].question);
   arithmetic_expected_request=cases[i].question;
   arithmetic_expected_answer=cases[i].expected;
   check(response_handler(&request,sizeof(request),&answer,sizeof(answer),&used,NULL)==STNLABZ_MODULE_OK&&used==sizeof(answer)&&strcmp(answer.answer,cases[i].expected)==0,cases[i].question);
  }
 }

 /* [AI:GPT-6 | 2026-10-09] A missing arithmetic provider must not
  * fall through to Corpus and fabricate an answer. */
 {
  memset(&request,0,sizeof(request));memset(&answer,0,sizeof(answer));used=0;
  snprintf(request.question,sizeof(request.question),"Digit what is 2 plus 2?");
  arithmetic_expected_request="Digit what is 2 plus 2?";
  arithmetic_available=0;
  check(response_handler(&request,sizeof(request),&answer,sizeof(answer),&used,NULL)==STNLABZ_MODULE_OK&&
   used==sizeof(answer)&&strcmp(answer.answer,"Arithmetic service is unavailable.")==0,
   "missing arithmetic provider fails closed");
  arithmetic_available=1;
 }
 /* [AI:GPT-6 | 2026-10-09] A valid knowledge request with zero
  * retrieved evidence must not invoke outbound factual validation. */
 memset(&request,0,sizeof(request));
 memset(&answer,0,sizeof(answer));
 used=0;corpus_queries=0;outbound_validations=0;
 snprintf(request.question,sizeof(request.question),"What is the mission?");
 check(response_handler(&request,sizeof(request),&answer,sizeof(answer),
       &used,NULL)==STNLABZ_MODULE_OK&&used==sizeof(answer)&&
       answer.answered&&strstr(answer.answer,"don't have enough grounded information")!=NULL,
       "missing evidence reports uncertainty");
 check(corpus_queries==1&&outbound_validations==0,
       "missing evidence does not enter outbound factual validation");
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
    check(descriptor != NULL && descriptor->version_major == 1 && descriptor->version_minor == 7 && descriptor->version_patch == 9, "internal version is 1.7.9");
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
