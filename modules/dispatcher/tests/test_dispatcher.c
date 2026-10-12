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
#include "dsd.h"

static stnlabz_module_service_handler_fn dispatcher_handler;
static stnlabz_module_service_handler_fn scoped_handler;
static stnlabz_module_service_handler_fn scoped_context_handler;
static stnlabz_module_service_handler_fn observed_handler;
static unsigned response_calls;
static unsigned dsd_calls;
static digit_intent_class_t chosen_intent;
static unsigned int chosen_established=1;
static const char *chosen_context_subject;
static unsigned int lesson_calls=0;
static unsigned int arithmetic_calls=0;
static unsigned int learned_social_fixture=0;
static unsigned int intent_invocations=0;

static int register_dispatcher(const char *name,stnlabz_module_service_handler_fn fn,void *ctx){
 (void)ctx;
 if(!strcmp(name,DIGIT_DISPATCHER_OBSERVED_CONTEXT_SERVICE)){observed_handler=fn;return 1;}
 if(!strcmp(name,DIGIT_DISPATCHER_SCOPED_SERVICE)){scoped_handler=fn;return 1;}
 if(!strcmp(name,DIGIT_DISPATCHER_SCOPED_CONTEXT_SERVICE)){scoped_context_handler=fn;return 1;}
 if(strcmp(name,DIGIT_DISPATCHER_SERVICE))return 0;
 dispatcher_handler=fn;return 1;
}
static int unregister_dispatcher(const char *name,void *ctx){
 (void)ctx;return !strcmp(name,DIGIT_DISPATCHER_SERVICE)||
 !strcmp(name,DIGIT_DISPATCHER_SCOPED_SERVICE)||
 !strcmp(name,DIGIT_DISPATCHER_SCOPED_CONTEXT_SERVICE)||
 !strcmp(name,DIGIT_DISPATCHER_OBSERVED_CONTEXT_SERVICE);
}
static stnlabz_module_result_t mock_invoke(const char *name,const void *input,size_t isize,
 void *output,size_t osize,size_t *used){
 if(!strcmp(name,DIGIT_DSD_SERVICE)){
  const digit_dsd_request_t *req=input;
  digit_dsd_result_t *out=output;
  if(isize!=sizeof(*req)||osize<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  dsd_calls++;
  memset(out,0,sizeof(*out));
  if(req->stage_count){
   out->finding=DIGIT_DSD_OBSERVED;
   snprintf(out->report,sizeof(out->report),"Observed processing boundary at response: NO_CONVERSATIONAL_COMPOSITION.");
  }else{
   out->finding=DIGIT_DSD_UNKNOWN;
   snprintf(out->report,sizeof(out->report),"Insufficient verified stage evidence.");
  }
  *used=sizeof(*out);return STNLABZ_MODULE_OK;
 }
 if(!strcmp(name,DIGIT_INTERPRETATION_SERVICE)){
  const digit_interpretation_request_t *in=input;
  digit_interpretation_result_t *out=output;
  if(isize!=sizeof(*in)||osize<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  memset(out,0,sizeof(*out));out->resolved=1;
  snprintf(out->normalized,sizeof(out->normalized),"%s",in->text);
  if(learned_social_fixture&&!strcmp(in->text,"purrs")){
   snprintf(out->normalized,sizeof(out->normalized),"affection");
   out->substitutions=1;
  }
  *used=sizeof(*out);return STNLABZ_MODULE_OK;
 }
 if(!strcmp(name,DIGIT_INTENT_SERVICE)){
  digit_intent_result_t *out=output;
  if(osize<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  intent_invocations++;
  memset(out,0,sizeof(*out));out->intent=chosen_intent;
  if(learned_social_fixture&&isize==sizeof(digit_intent_request_t)&&
     !strcmp(((const digit_intent_request_t *)input)->text,"purrs")){
   out->intent=DIGIT_INTENT_CONVERSATION;
   out->target=DIGIT_INTENT_TARGET_SOCIAL;
   out->established=1;
   *used=sizeof(*out);return STNLABZ_MODULE_OK;
  }
  out->target=chosen_intent==DIGIT_INTENT_ACTION?DIGIT_INTENT_TARGET_CAPABILITY:
              DIGIT_INTENT_TARGET_KNOWLEDGE;
  out->established=chosen_established;
  if(chosen_context_subject)snprintf(out->subject,sizeof(out->subject),"%s",chosen_context_subject);
  *used=sizeof(*out);return STNLABZ_MODULE_OK;
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
 if(!strcmp(name,DIGIT_RESPONSE_OBSERVED_SERVICE)){
  digit_response_observed_result_t *out=output;
  if(osize<sizeof(*out))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
  response_calls++;if(learned_social_fixture && strstr(((const digit_response_request_t *)input)->question,"REQUEST: purrs"))learned_social_request_preserved=1;
   memset(out,0,sizeof(*out));out->answered=1;
  if(strstr(((const digit_response_request_t *)input)->question,"You are Stoopid")){
   snprintf(out->answer,sizeof(out->answer),"I don\'t have enough grounded information to answer that.");
   out->retrieved_count=2;out->selected_count=1;
   out->diagnostic_phase=DIGIT_RESPONSE_PHASE_NO_COMPOSITION;
  }else{
   snprintf(out->answer,sizeof(out->answer),"%s",
    strstr(((const digit_response_request_t *)input)->question,"solve 3*x - 6 = 0")?"x = 2.":"Bounded response");
   out->diagnostic_phase=DIGIT_RESPONSE_PHASE_APPROVED;
  }
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
 if(!d||strcmp(d->id,"dispatcher")||d->version_major!=1||d->version_minor!=4||d->version_patch!=3||!d->qualify){
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
  /* Learned social expression survives lexical normalization, but cannot
   * elevate a normalized UNKNOWN into an operational capability. */
  {
   digit_dispatcher_observed_context_request_t in={0};
   digit_dispatcher_observed_result_t out={0};
   size_t used=0;
   snprintf(in.context.scoped.actor,sizeof(in.context.scoped.actor),"authenticated-user");
   in.context.scoped.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
   snprintf(in.context.scoped.request,sizeof(in.context.scoped.request),"purrs");
   learned_social_fixture=1;chosen_intent=DIGIT_INTENT_UNKNOWN;
   chosen_established=0;intent_invocations=0;response_calls=0;
   if(!observed_handler||observed_handler(&in,sizeof(in),&out,
       sizeof(out),&used,NULL)!=STNLABZ_MODULE_OK||
      used!=sizeof(out)||out.diagnostic.intent_class!=DIGIT_INTENT_CONVERSATION||
      !out.diagnostic.intent_established||intent_invocations!=2||
      response_calls!=1||!learned_social_request_preserved){
    fprintf(stderr,"Learned social fallback FAILED\\n");return 1;
   }
   learned_social_fixture=0;
  }
  {
   digit_dispatcher_scoped_context_request_t req={0};
   digit_dispatcher_result_t out={0};
   size_t used=0;
   snprintf(req.scoped.request,sizeof(req.scoped.request),"Digit, diagnose my last request.");
   snprintf(req.scoped.actor,sizeof(req.scoped.actor),"authenticated-user");
   req.scoped.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
   snprintf(req.previous_user_message,sizeof(req.previous_user_message),"You are Stoopid");
   snprintf(req.previous_answer,sizeof(req.previous_answer),"I don't have enough grounded information to answer that.");
   dsd_calls=0;
   if(!scoped_context_handler||scoped_context_handler(&req,sizeof(req),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_OK||
      used!=sizeof(out)||dsd_calls!=0||!strstr(out.answer,"PREVIOUS_TRACE_UNAVAILABLE")){
    fprintf(stderr,"Authenticated DSD routing FAILED\\n");return 1;
   }
   /* Exercise a real request through the mocked stage services and
    * carry only that authenticated turn's observations forward. */
   {
    digit_dispatcher_observed_context_request_t turn={0};
    digit_dispatcher_observed_result_t first={0};
    size_t first_used=0;
    snprintf(turn.context.scoped.request,sizeof(turn.context.scoped.request),"You are Stoopid");
    snprintf(turn.context.scoped.actor,sizeof(turn.context.scoped.actor),"authenticated-user");
    turn.context.scoped.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
    chosen_intent=DIGIT_INTENT_CONVERSATION;chosen_established=1;
    if(observed_handler(&turn,sizeof(turn),&first,sizeof(first),&first_used,NULL)!=STNLABZ_MODULE_OK||
       first_used!=sizeof(first)||first.diagnostic_valid!=1||
       first.diagnostic.stage_count!=3||
       strcmp(first.diagnostic.stages[0].module,"interpretation")||
       strcmp(first.diagnostic.stages[1].module,"intent")||
       strcmp(first.diagnostic.stages[2].reason,"NO_CONVERSATIONAL_COMPOSITION")||
       first.diagnostic.retrieved!=2||first.diagnostic.selected!=1){
     fprintf(stderr,"DSD observed-stage capture FAILED\\n");return 1;
    }
    digit_dispatcher_observed_context_request_t diagnosed={0};
    digit_dispatcher_observed_result_t report={0};
    diagnosed.context=req;
    diagnosed.previous_diagnostic_valid=1;
    diagnosed.previous_diagnostic=first.diagnostic;
    dsd_calls=0;used=0;
    if(observed_handler(&diagnosed,sizeof(diagnosed),&report,sizeof(report),&used,NULL)!=STNLABZ_MODULE_OK||
       used!=sizeof(report)||dsd_calls!=1||
       !strstr(report.answer,"NO_CONVERSATIONAL_COMPOSITION")){
     fprintf(stderr,"DSD prior-turn correlation FAILED\\n");return 1;
    }

   }
   /* Explicit diagnostics never read unauthenticated or channel context. */
   {
    digit_dispatcher_request_t legacy={0};
    digit_dispatcher_result_t blocked={0};
    size_t n=0;
    snprintf(legacy.request,sizeof(legacy.request),"Digit, diagnose my last request.");
    dsd_calls=0;
    if(dispatcher_handler(&legacy,sizeof(legacy),&blocked,sizeof(blocked),&n,NULL)!=STNLABZ_MODULE_OK||
       dsd_calls!=0||!strstr(blocked.answer,"authenticated private")){
     fprintf(stderr,"DSD unscoped isolation FAILED\\n");return 1;
    }
   }
   req.scoped.scope_kind=DIGIT_DISPATCHER_SCOPE_CHANNEL;
   snprintf(req.scoped.organization,sizeof(req.scoped.organization),"org");
   snprintf(req.scoped.project,sizeof(req.scoped.project),"project");
   snprintf(req.scoped.channel,sizeof(req.scoped.channel),"channel");
   dsd_calls=0;
   if(scoped_context_handler(&req,sizeof(req),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_ERR_INVALID_ARGUMENT||
      dsd_calls!=0){
    fprintf(stderr,"DSD channel isolation FAILED\\n");return 1;
   }
   req.scoped.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
   req.scoped.organization[0]=0;req.scoped.project[0]=0;req.scoped.channel[0]=0;
   req.previous_user_message[0]=0;
   dsd_calls=0;used=0;
   if(scoped_context_handler(&req,sizeof(req),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_OK||
      dsd_calls!=0||!strstr(out.answer,"No diagnosis is available")){
    fprintf(stderr,"DSD missing-context gate FAILED\\n");return 1;
   }
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
  {
   digit_dispatcher_scoped_context_request_t ctx={0};
   digit_dispatcher_result_t out={0};
   size_t used=0;
   chosen_intent=DIGIT_INTENT_CONTEXT;chosen_established=1;
   chosen_context_subject="prior_user_message";
   snprintf(ctx.scoped.actor,sizeof(ctx.scoped.actor),"verified_user");
   snprintf(ctx.scoped.request,sizeof(ctx.scoped.request),"What did I just tell you?");
   ctx.scoped.scope_kind=DIGIT_DISPATCHER_SCOPE_PRIVATE;
   snprintf(ctx.previous_user_message,sizeof(ctx.previous_user_message),"I created the project.");
   if(!scoped_context_handler||
      scoped_context_handler(&ctx,sizeof(ctx),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_OK||
      strcmp(out.answer,"I created the project.")!=0){
    fprintf(stderr,"Dispatcher previous-user-message context FAILED\\n");return 1;
   }
   ctx.scoped.scope_kind=DIGIT_DISPATCHER_SCOPE_CHANNEL;
   if(scoped_context_handler(&ctx,sizeof(ctx),&out,sizeof(out),&used,NULL)!=STNLABZ_MODULE_ERR_INVALID_ARGUMENT){
    fprintf(stderr,"Dispatcher private history escaped into channel scope\\n");return 1;
   }
   chosen_context_subject=NULL;
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
