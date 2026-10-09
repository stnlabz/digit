#include <stdio.h>
#include <string.h>
#include "dispatcher.h"

/* [AI:GPT-6 | 2026-10-09] Run executable module qualification and
 * validate the returned evidence rather than assuming declared passes. */
#include "intent.h"
#include "interpretation.h"
#include "response.h"
#include "lesson.h"
#include "arithmetic.h"

static stnlabz_module_service_handler_fn dispatcher_handler;
static stnlabz_module_service_handler_fn scoped_handler;
static unsigned response_calls;
static digit_intent_class_t chosen_intent;
static unsigned int chosen_established=1;
static unsigned int lesson_calls=0;
static unsigned int arithmetic_calls=0;

static int register_dispatcher(const char *name,stnlabz_module_service_handler_fn fn,void *ctx){
 (void)ctx;
 if(!strcmp(name,DIGIT_DISPATCHER_SCOPED_SERVICE)){scoped_handler=fn;return 1;}
 if(strcmp(name,DIGIT_DISPATCHER_SERVICE))return 0;
 dispatcher_handler=fn;return 1;
}
static int unregister_dispatcher(const char *name,void *ctx){
 (void)ctx;return !strcmp(name,DIGIT_DISPATCHER_SERVICE)||!strcmp(name,DIGIT_DISPATCHER_SCOPED_SERVICE);
}
static stnlabz_module_result_t mock_invoke(const char *name,const void *input,size_t isize,
 void *output,size_t osize,size_t *used){
 if(!strcmp(name,DIGIT_INTERPRETATION_SERVICE)){
  const digit_interpretation_request_t *in=input;
  digit_interpretation_result_t *out=output;
  if(isize!=sizeof(*in)||osize<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  memset(out,0,sizeof(*out));out->resolved=1;
  snprintf(out->normalized,sizeof(out->normalized),"%s",in->text);
  *used=sizeof(*out);return STNLABZ_MODULE_OK;
 }
 if(!strcmp(name,DIGIT_INTENT_SERVICE)){
  digit_intent_result_t *out=output;
  if(osize<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  memset(out,0,sizeof(*out));out->intent=chosen_intent;
  out->target=chosen_intent==DIGIT_INTENT_ACTION?DIGIT_INTENT_TARGET_CAPABILITY:
              DIGIT_INTENT_TARGET_KNOWLEDGE;
  out->established=chosen_established;*used=sizeof(*out);return STNLABZ_MODULE_OK;
 }
 if(!strcmp(name,DIGIT_LESSON_INGEST_SERVICE)){
  lesson_calls++;
  return STNLABZ_MODULE_ERR_NOT_FOUND;
 }
 if(!strcmp(name,DIGIT_ARITHMETIC_SERVICE)){
  const digit_arithmetic_request_t *in=input;
  digit_arithmetic_result_t *out=output;
  if(isize!=sizeof(*in)||osize<sizeof(*out)||strcmp(in->expression,"solve 3*x - 6 = 0"))
   return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  arithmetic_calls++;
  memset(out,0,sizeof(*out));out->status=DIGIT_ARITHMETIC_OK;
  snprintf(out->decimal_answer,sizeof(out->decimal_answer),"x = 2.");
  *used=sizeof(*out);return STNLABZ_MODULE_OK;
 }
 if(!strcmp(name,DIGIT_RESPONSE_SERVICE)){
  digit_response_result_t *out=output;
  if(osize<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  response_calls++;memset(out,0,sizeof(*out));out->answered=1;
  snprintf(out->answer,sizeof(out->answer),"%s",
   strstr(((const digit_response_request_t *)input)->question,"solve 3*x - 6 = 0")?"x = 2.":"Bounded response");
  *used=sizeof(*out);return STNLABZ_MODULE_OK;
 }
 return STNLABZ_MODULE_ERR_NOT_FOUND;
}
static int dispatch_case(const char *request,digit_intent_class_t intent,
 int expect_response,const char *contains){
 digit_dispatcher_request_t in;
 digit_dispatcher_result_t out;size_t used=0;
 memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));
 snprintf(in.request,sizeof(in.request),"%s",request);
 chosen_intent=intent;chosen_established=1;response_calls=0;lesson_calls=0;
 if(dispatcher_handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_OK||
    used!=sizeof(out)||out.answered!=1)return 0;
 return response_calls==(unsigned)expect_response&&
        strstr(out.answer,contains)!=NULL;
}
int main(void){
 const stnlabz_module_descriptor_t *d=stnlabz_module_get_descriptor();
 stnlabz_module_qualification_result_t q;
 if(!d||strcmp(d->id,"dispatcher")||d->version_major!=1||d->version_minor!=3||d->version_patch!=3||!d->qualify){
  fprintf(stderr,"Dispatcher descriptor invalid\\n");return 1;
 }
 memset(&q,0,sizeof(q));
 if(d->qualify(&q)!=STNLABZ_MODULE_OK||q.tests_executed<10||
    q.tests_passed!=q.tests_executed||q.tests_failed!=0||
    q.negative_test_executed<1||q.negative_test_passed!=q.negative_test_executed){
  fprintf(stderr,"Dispatcher qualification FAILED (%u/%u, negative %d/%d)\\n",
    q.tests_passed,q.tests_executed,q.negative_test_passed,q.negative_test_executed);
  return 1;
 }
 {
  stnlabz_module_host_t host={0};
  host.register_service=register_dispatcher;
  host.unregister_service=unregister_dispatcher;
  host.invoke_service=mock_invoke;
  if(!d->start||d->start(&host)!=STNLABZ_MODULE_OK||!dispatcher_handler){
   fprintf(stderr,"Dispatcher mock registration FAILED\\n");return 1;
  }
  if(!dispatch_case("Write a C function that adds two integers.",DIGIT_INTENT_ACTION,0,
                    "cannot execute or generate")||
     !dispatch_case("Compare C and Python",DIGIT_INTENT_COMPARE,1,"Bounded response")||
     !dispatch_case("Who are you?",DIGIT_INTENT_FACT,1,"Bounded response")||
     !dispatch_case("Inspect the source",DIGIT_INTENT_ACTION,0,"can't identify that source project")||
     !dispatch_case("ingest lesson-one",DIGIT_INTENT_ACTION,0,"couldn't ingest lesson")||
     lesson_calls!=1||
     !dispatch_case("ingest lesson-one",DIGIT_INTENT_FACT,1,"Bounded response")||
     lesson_calls!=0){
   fprintf(stderr,"Dispatcher end-to-end routing FAILED\\n");return 1;
  }
  {
   digit_dispatcher_request_t input={0};
   digit_dispatcher_result_t output={0};
   size_t used=0;
   snprintf(input.request,sizeof(input.request),"ingest lesson-one");
   chosen_intent=DIGIT_INTENT_ACTION;chosen_established=0;
   lesson_calls=0;
   if(dispatcher_handler(&input,sizeof(input),&output,sizeof(output),&used,NULL)!=STNLABZ_MODULE_OK||
      lesson_calls!=0||strstr(output.answer,"can't establish")==NULL){
    fprintf(stderr,"Dispatcher unestablished action gate FAILED\\n");return 1;
   }
  }
  /* [AI:GPT-6 | 2026-10-09] No unscoped cross-operator follow-up.
   * Until authenticated identity reaches Dispatcher, prior equation state
   * will not be retained or reused, even after a successful answer. */
  if(!dispatch_case("verify your answer",DIGIT_INTENT_UNKNOWN,0,
                    "without authenticated, account-bound conversation context") ||
     !dispatch_case("solve 3*x - 6 = 0",DIGIT_INTENT_FACT,1,"x = 2.") ||
     !dispatch_case("verify your answer",DIGIT_INTENT_UNKNOWN,0,
                    "without authenticated, account-bound conversation context") ||
     arithmetic_calls!=0 ||
     !dispatch_case("Who are you?",DIGIT_INTENT_FACT,1,"Bounded response") ||
     !dispatch_case("verify your answer",DIGIT_INTENT_UNKNOWN,0,
                    "without authenticated, account-bound conversation context")){
   fprintf(stderr,"Dispatcher unscoped follow-up rejection FAILED\\n");return 1;
  }
  /* [AI:GPT-6 | 2026-10-09] Scoped ABI acceptance and denial. */
  {
   digit_dispatcher_scoped_request_t in={0};
   digit_dispatcher_result_t out={0};
   size_t used=0;
   snprintf(in.request,sizeof(in.request),"Who are you?");
   snprintf(in.actor,sizeof(in.actor),"account_1");
   in.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
   chosen_intent=DIGIT_INTENT_FACT;chosen_established=1;
   if(!scoped_handler||
      scoped_handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_OK||
      used!=sizeof(out)||strstr(out.answer,"Bounded response")==NULL){
    fprintf(stderr,"Dispatcher scoped private route FAILED\\n");return 1;
   }
   in.scope_kind=DIGIT_DISPATCHER_SCOPE_CHANNEL;
   if(scoped_handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_ERR_INVALID_ARGUMENT){
    fprintf(stderr,"Dispatcher incomplete channel scope accepted\\n");return 1;
   }
   snprintf(in.organization,sizeof(in.organization),"stn-labz");
   snprintf(in.project,sizeof(in.project),"development");
   snprintf(in.channel,sizeof(in.channel),"general");
   if(scoped_handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_OK){
    fprintf(stderr,"Dispatcher valid channel scope rejected\\n");return 1;
   }
   in.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
   if(scoped_handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_ERR_INVALID_ARGUMENT){
    fprintf(stderr,"Dispatcher contaminated private scope accepted\\n");return 1;
   }
   memset(&in,0,sizeof(in));
   snprintf(in.request,sizeof(in.request),"Who are you?");
   in.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
   if(scoped_handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_ERR_INVALID_ARGUMENT){
    fprintf(stderr,"Dispatcher anonymous private scope accepted\\n");return 1;
   }
  }
  if(!d->stop||d->stop()!=STNLABZ_MODULE_OK){
   fprintf(stderr,"Dispatcher mock shutdown FAILED\\n");return 1;
  }
 }
 printf("Dispatcher qualification: %u/%u checks, negative %d/%d — PASS\\n",
    q.tests_passed,q.tests_executed,q.negative_test_passed,q.negative_test_executed);
 return 0;
}
