#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dsd.h"

/* [AI:GPT-6 | 2026-10-10] Deterministic, stateless incident analysis.
 * Observations must arrive from an authorized caller through the host's
 * existing module service boundary. DSD cannot inspect private sessions,
 * write to runtime state, or promote a repair. */
static const stnlabz_module_host_t *dsd_host;
static int terminated(const char *value,size_t size){
    return memchr(value,0,size)!=NULL;
}
static int code_valid(const char *value,size_t size){
    size_t i;
    if(!terminated(value,size))return 0;
    for(i=0;value[i];++i){
        unsigned char ch=(unsigned char)value[i];
        if(!(isalnum(ch)||ch=='_'||ch=='-'||ch=='.'))return 0;
    }
    return 1;
}
/* [AI:GPT-6 | 2026-10-11] Source-relative investigation targets,
 * NOT claims of source fault. Only known module IDs are mapped, and the
 * exact function/line remains unverified until source-level investigation. */
static const char *source_target(const char *module){
    static const struct {const char *module,*path;} known[]={
        {"interpretation","modules/interpretation/src/interpretation.c"},
        {"intent","modules/intent/src/intent.c"},
        {"response","modules/response/src/response.c"},
        {"validator","modules/validator/src/validator.c"},
        {"dispatcher","modules/dispatcher/src/dispatcher.c"},
        {"dsd","modules/dsd/src/dsd.c"}
    };
    size_t i;
    for(i=0;i<sizeof(known)/sizeof(known[0]);++i)
        if(strcmp(module,known[i].module)==0)return known[i].path;
    return NULL;
}
static void engineering_handoff(digit_dsd_engineering_result_t *out){
    const char *target;
    if(out->diagnostic.finding==DIGIT_DSD_UNKNOWN || !out->diagnostic.affected_module[0]){
        snprintf(out->symptom,sizeof(out->symptom),
                 "Insufficient module-stage evidence to establish an affected module");
        out->location_status=DIGIT_DSD_LOCATION_UNRESOLVED;
        out->source_location[0]=0;
    }else{
        snprintf(out->symptom,sizeof(out->symptom),
                 "Processing stage reported %s",out->diagnostic.reason[0]?out->diagnostic.reason:"UNSPECIFIED_FAILURE");
        target=source_target(out->diagnostic.affected_module);
        if(target){
            snprintf(out->source_location,sizeof(out->source_location),"%s",target);
            out->location_status=DIGIT_DSD_LOCATION_INVESTIGATE;
        }else{
            out->source_location[0]=0;
            out->location_status=DIGIT_DSD_LOCATION_UNRESOLVED;
        }
    }
    {char narrative[DIGIT_DSD_REPORT_MAX];
     snprintf(narrative,sizeof(narrative),"%s",out->diagnostic.report);
     snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
         "Module: %s. Symptom: %s. Source investigation location: %s. "
         "Location evidence: %s. Diagnostic reason: %s. %.260s",
         out->diagnostic.affected_module[0]?out->diagnostic.affected_module:"NOT_IDENTIFIED",
         out->symptom,
         out->source_location[0]?out->source_location:"NOT_ESTABLISHED",
         out->location_status==DIGIT_DSD_LOCATION_INVESTIGATE?
             "INVESTIGATION_TARGET_NOT_VERIFIED_ROOT_CAUSE":"UNRESOLVED",
         out->diagnostic.reason,narrative);
    }
}
static int inspect(const digit_dsd_request_t *in,digit_dsd_engineering_result_t *out){
    size_t i;
    unsigned int ok=0;
    if(!in||!out||in->stage_count>DIGIT_DSD_STAGE_COUNT||
       in->intent_observed>1||in->intent_established>1||
       in->lookup_count>DIGIT_DSD_LOOKUP_MAX||
       !terminated(in->interpretation_reason,sizeof(in->interpretation_reason))||
       !terminated(in->intent_reason,sizeof(in->intent_reason))||
       !terminated(in->intent_subject,sizeof(in->intent_subject))||
       in->candidate_constructed>1||in->outbound_approved>1||
       in->selected>in->retrieved||in->outbound_approved&&!in->candidate_constructed)
        return 0;
    memset(out,0,sizeof(*out));
    for(i=0;i<in->stage_count;i++){
        const digit_dsd_stage_t *stage=&in->stages[i];
        if(stage->state>DIGIT_DSD_UNAVAILABLE||
           !code_valid(stage->module,sizeof(stage->module))||
           !code_valid(stage->reason,sizeof(stage->reason))||
           !stage->module[0])return 0;
        if(stage->state==DIGIT_DSD_OK)ok++;
        if(stage->state==DIGIT_DSD_FAILED||stage->state==DIGIT_DSD_UNAVAILABLE){
            if(out->diagnostic.finding==DIGIT_DSD_UNKNOWN){
                out->diagnostic.finding=DIGIT_DSD_OBSERVED;
                snprintf(out->diagnostic.affected_module,sizeof(out->diagnostic.affected_module),"%s",stage->module);
                snprintf(out->diagnostic.reason,sizeof(out->diagnostic.reason),"%s",
                         stage->reason[0]?stage->reason:"STAGE_UNAVAILABLE");
            }
        }
    }
    if(out->diagnostic.finding==DIGIT_DSD_OBSERVED){
        snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
                 "Observed processing boundary at %s: %s. Retrieved %u record(s), selected %u. This identifies the reported boundary, not a verified underlying root cause. No repair or deployment is authorized.",
                 out->diagnostic.affected_module,out->diagnostic.reason,in->retrieved,in->selected);
    }else if(in->retrieved>0&&in->selected==0){
        out->diagnostic.finding=DIGIT_DSD_INFERRED;
        snprintf(out->diagnostic.affected_module,sizeof(out->diagnostic.affected_module),"response");
        snprintf(out->diagnostic.reason,sizeof(out->diagnostic.reason),"NO_EVIDENCE_SELECTED");
        snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
                 "Corpus returned %u record(s), but Response selected none. This locates an evidence-selection boundary; the underlying cause is unknown.",
                 in->retrieved);
    }else if(in->selected>0&&!in->candidate_constructed){
        out->diagnostic.finding=DIGIT_DSD_INFERRED;
        snprintf(out->diagnostic.affected_module,sizeof(out->diagnostic.affected_module),"response");
        snprintf(out->diagnostic.reason,sizeof(out->diagnostic.reason),"NO_ANSWER_CANDIDATE");
        snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
                 "Response selected %u record(s) but no answer candidate was constructed. Inspect the learned relation and composition rules; root cause not established.",
                 in->selected);
    }else if(in->candidate_constructed&&!in->outbound_approved){
        out->diagnostic.finding=DIGIT_DSD_INFERRED;
        snprintf(out->diagnostic.affected_module,sizeof(out->diagnostic.affected_module),"validator");
        snprintf(out->diagnostic.reason,sizeof(out->diagnostic.reason),"CANDIDATE_NOT_APPROVED");
        snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
                 "A candidate was constructed but not approved. Check the recorded Validator disposition; rejection versus unavailable validation is not established.");
    }else if(in->stage_count>0&&ok==in->stage_count&&in->outbound_approved){
        out->diagnostic.finding=DIGIT_DSD_OBSERVED;
        snprintf(out->diagnostic.reason,sizeof(out->diagnostic.reason),"NO_REPORTED_FAILURE");
        snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
                 "The supplied stages report success and the outbound candidate was approved. This is not proof of live conversational acceptance.");
    }else{
        out->diagnostic.finding=DIGIT_DSD_UNKNOWN;
        snprintf(out->diagnostic.reason,sizeof(out->diagnostic.reason),"INSUFFICIENT_EVIDENCE");
        snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
                 "Available telemetry: %u stage(s), %u reporting success, retrieved %u record(s), selected %u. No failed boundary was established. Additional stage telemetry is required to determine the underlying cause.",
                 in->stage_count,ok,in->retrieved,in->selected);
    }
    engineering_handoff(out);
    /* [AI:GPT-6 | 2026-10-12] Explain the recorded decision, not merely
     * the last stage code. Never claim a Corpus lookup ran if Intent stopped
     * dispatch before Response. State unresolved evidence explicitly. */
    if(in->intent_observed){
        char prior[DIGIT_DSD_REPORT_MAX];
        const char *classification=in->intent_class==0?"UNKNOWN":
            in->intent_class==10?"AMBIGUOUS":"CLASSIFIED";
        snprintf(prior,sizeof(prior),"%s",out->diagnostic.report);
        if(in->intent_class==0||in->intent_class==10){
            snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
              "Intent: %s (established=%u), reason: %.110s. "
              "Interpretation substitutions: %u, reason: %.70s. "
              "Dispatcher stopped before Response retrieval (NOT QUERIED). "
              "Lexical lookups follow. Individual Corpus record rejection rules were not traced. "
              "Intent: modules/intent/src/intent.c; gate: modules/dispatcher/src/dispatcher.c.",
              classification,in->intent_established,in->intent_reason,
              in->interpretation_substitutions,in->interpretation_reason);
        }else{
            snprintf(out->diagnostic.report,sizeof(out->diagnostic.report),
              "Intent classified the interpreted request (class=%u, target=%u, established=%u; "
              "subject: %.100s; reason: %.150s). Interpretation substitutions=%u. %.260s",
              in->intent_class,in->intent_target,in->intent_established,in->intent_subject,
              in->intent_reason,in->interpretation_substitutions,prior);
        }
    }
    if(in->lookup_count){
        size_t used=strlen(out->diagnostic.report),i;
        for(i=0;i<in->lookup_count&&used+110<sizeof(out->diagnostic.report);++i){
            const digit_dsd_lookup_t *ev=&in->lookups[i];
            int written=snprintf(out->diagnostic.report+used,sizeof(out->diagnostic.report)-used,
                " Term %.40s -> Interpretation/Corpus search: %s, %u result(s), "
                "decision=%.42s, record=%.42s.",ev->term,
                ev->query_ok?"COMPLETED":"UNAVAILABLE",ev->returned,
                ev->decision[0]?ev->decision:"NOT_RECORDED",
                ev->accepted?ev->selected_record_id:"NONE");
            if(written<0)break;
            if((size_t)written>=sizeof(out->diagnostic.report)-used)break;
            used+=(size_t)written;
        }
    }
    return 1;
}
static stnlabz_module_result_t diagnose_service(const void *request,size_t request_size,
    void *response,size_t response_size,size_t *response_used,void *context){
    digit_dsd_engineering_result_t result;
    (void)context;
    if(!request||request_size!=sizeof(digit_dsd_request_t)||!response||
       response_size<sizeof(result.diagnostic)||!response_used)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!inspect((const digit_dsd_request_t *)request,&result))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memcpy(response,&result.diagnostic,sizeof(result.diagnostic));
    *response_used=sizeof(result.diagnostic);
    return STNLABZ_MODULE_OK;
}
/* New clients opt in to structured engineering fields. Old Dispatcher
 * clients continue using the unchanged dsd.diagnose wire schema. */
static stnlabz_module_result_t engineering_service(const void *request,size_t request_size,
    void *response,size_t response_size,size_t *response_used,void *context){
    digit_dsd_engineering_result_t result;
    (void)context;
    if(!request||request_size!=sizeof(digit_dsd_request_t)||!response||
       response_size<sizeof(result)||!response_used)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!inspect((const digit_dsd_request_t *)request,&result))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memcpy(response,&result,sizeof(result));*response_used=sizeof(result);
    return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t dsd_qualify(stnlabz_module_qualification_result_t *result){
    digit_dsd_request_t q={0};
    digit_dsd_engineering_result_t answer;
    unsigned int passed=0;
    if(!result)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result,0,sizeof(*result));
    q.retrieved=2;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.diagnostic.finding==DIGIT_DSD_INFERRED&&
        strcmp(answer.diagnostic.reason,"NO_EVIDENCE_SELECTED")==0);
    q.selected=1;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.diagnostic.finding==DIGIT_DSD_INFERRED&&
        strcmp(answer.diagnostic.reason,"NO_ANSWER_CANDIDATE")==0);
    q.candidate_constructed=1;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.diagnostic.finding==DIGIT_DSD_INFERRED&&
        strcmp(answer.diagnostic.reason,"CANDIDATE_NOT_APPROVED")==0);
    q.outbound_approved=1;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.diagnostic.finding==DIGIT_DSD_UNKNOWN);
    q.stage_count=1;
    snprintf(q.stages[0].module,sizeof(q.stages[0].module),"interpretation");
    snprintf(q.stages[0].reason,sizeof(q.stages[0].reason),"SERVICE_TIMEOUT");
    q.stages[0].state=DIGIT_DSD_FAILED;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.diagnostic.finding==DIGIT_DSD_OBSERVED&&
        strcmp(answer.diagnostic.affected_module,"interpretation")==0);
    q.stages[0].state=DIGIT_DSD_OK;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.diagnostic.finding==DIGIT_DSD_OBSERVED&&
        strcmp(answer.diagnostic.reason,"NO_REPORTED_FAILURE")==0);
    q.stage_count=DIGIT_DSD_STAGE_COUNT+1;
    passed+=(unsigned)!inspect(&q,&answer);
    q.stage_count=1;
    q.stages[0].state=DIGIT_DSD_FAILED;
    snprintf(q.stages[0].reason,sizeof(q.stages[0].reason),"INVALID CODE");
    passed+=(unsigned)!inspect(&q,&answer);
    q.stages[0].reason[0]=0;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.diagnostic.finding==DIGIT_DSD_OBSERVED&&
        strcmp(answer.diagnostic.reason,"STAGE_UNAVAILABLE")==0);
    q.stage_count=0;q.retrieved=1;q.selected=2;
    passed+=(unsigned)!inspect(&q,&answer);
    /* Independent regression cases for the new engineering handoff. */
    memset(&q,0,sizeof(q));q.retrieved=2;
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.diagnostic.affected_module,"response")==0&&
      strcmp(answer.source_location,"modules/response/src/response.c")==0&&
      answer.location_status==DIGIT_DSD_LOCATION_INVESTIGATE&&
      strstr(answer.diagnostic.report,"Module: response.")!=NULL);
    memset(&q,0,sizeof(q));q.stage_count=1;
    snprintf(q.stages[0].module,sizeof(q.stages[0].module),"intent");
    snprintf(q.stages[0].reason,sizeof(q.stages[0].reason),"SERVICE_TIMEOUT");
    q.stages[0].state=DIGIT_DSD_FAILED;
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.source_location,"modules/intent/src/intent.c")==0&&
      strstr(answer.symptom,"SERVICE_TIMEOUT")!=NULL);
    snprintf(q.stages[0].module,sizeof(q.stages[0].module),"unmapped");
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.diagnostic.affected_module,"unmapped")==0&&
      !answer.source_location[0]&&
      answer.location_status==DIGIT_DSD_LOCATION_UNRESOLVED);
    memset(&q,0,sizeof(q));
    passed+=(unsigned)(inspect(&q,&answer)&&
      !answer.diagnostic.affected_module[0]&&!answer.source_location[0]&&
      answer.location_status==DIGIT_DSD_LOCATION_UNRESOLVED);
    memset(&q,0,sizeof(q));q.selected=1;q.retrieved=1;
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.source_location,"modules/response/src/response.c")==0&&
      strstr(answer.diagnostic.report,"NOT_VERIFIED_ROOT_CAUSE")!=NULL);
    /* [AI:GPT-6 | 2026-10-12] Recorded intent reason must survive the
     * previous-turn diagnostic path without inventing Corpus activity. */
    memset(&q,0,sizeof(q));q.stage_count=2;
    snprintf(q.stages[0].module,sizeof(q.stages[0].module),"interpretation");
    q.stages[0].state=DIGIT_DSD_OK;
    snprintf(q.stages[1].module,sizeof(q.stages[1].module),"intent");
    q.stages[1].state=DIGIT_DSD_FAILED;
    snprintf(q.stages[1].reason,sizeof(q.stages[1].reason),"INTENT_UNCLASSIFIED");
    q.intent_observed=1;q.intent_class=0;q.intent_established=0;
    snprintf(q.intent_reason,sizeof(q.intent_reason),"Intent is not deterministically established.");
    passed+=(unsigned)(inspect(&q,&answer)&&
       strstr(answer.diagnostic.report,"Intent is not deterministically established.")!=NULL&&
       strstr(answer.diagnostic.report,"NOT QUERIED")!=NULL);
    q.lookup_count=1;
    snprintf(q.lookups[0].term,sizeof(q.lookups[0].term),"wut");
    snprintf(q.lookups[0].selected_record_id,sizeof(q.lookups[0].selected_record_id),"lesson-42");
    q.lookups[0].returned=3;q.lookups[0].accepted=1;q.lookups[0].query_ok=1;
    passed+=(unsigned)(inspect(&q,&answer)&&
        strstr(answer.diagnostic.report,"Term wut -> Interpretation/Corpus search: COMPLETED")!=NULL&&
        strstr(answer.diagnostic.report,"lesson-42")!=NULL);
    result->tests_executed=17;
    result->tests_passed=passed;
    result->tests_failed=17-passed;
    result->negative_test_executed=3;
    result->negative_test_passed=passed==17?3:0;
    return passed==17?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_QUALIFICATION;
}
static stnlabz_module_result_t dsd_start(const stnlabz_module_host_t *host){
    if(!host||!host->register_service||!host->unregister_service)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!host->register_service(DIGIT_DSD_SERVICE,diagnose_service,NULL))
        return STNLABZ_MODULE_ERR_START_FAILED;
    if(!host->register_service(DIGIT_DSD_ENGINEERING_SERVICE,engineering_service,NULL)){
        (void)host->unregister_service(DIGIT_DSD_SERVICE,NULL);
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    dsd_host=host;
    if(host->send_message)(void)host->send_message("[DSD] read-only diagnostic service active");
    return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t dsd_stop(void){
    int ok=1;
    if(dsd_host&&dsd_host->unregister_service)
        ok=dsd_host->unregister_service(DIGIT_DSD_SERVICE,NULL);
    if(dsd_host&&dsd_host->unregister_service)
        if(!dsd_host->unregister_service(DIGIT_DSD_ENGINEERING_SERVICE,NULL))ok=0;
    dsd_host=NULL;
    return ok?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_STOP_FAILED;
}
static const stnlabz_module_descriptor_t descriptor={
    "dsd","Digit Self-Diagnostics",1,2,0,
    STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,
    dsd_qualify,dsd_start,dsd_stop
};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){
    return &descriptor;
}
