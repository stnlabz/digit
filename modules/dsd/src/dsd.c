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
static void engineering_handoff(digit_dsd_result_t *out){
    const char *target;
    if(out->finding==DIGIT_DSD_UNKNOWN || !out->affected_module[0]){
        snprintf(out->symptom,sizeof(out->symptom),
                 "Insufficient module-stage evidence to establish an affected module");
        out->location_status=DIGIT_DSD_LOCATION_UNRESOLVED;
        out->source_location[0]=0;
    }else{
        snprintf(out->symptom,sizeof(out->symptom),
                 "Processing stage reported %s",out->reason[0]?out->reason:"UNSPECIFIED_FAILURE");
        target=source_target(out->affected_module);
        if(target){
            snprintf(out->source_location,sizeof(out->source_location),"%s",target);
            out->location_status=DIGIT_DSD_LOCATION_INVESTIGATE;
        }else{
            out->source_location[0]=0;
            out->location_status=DIGIT_DSD_LOCATION_UNRESOLVED;
        }
    }
    {char narrative[DIGIT_DSD_REPORT_MAX];
     snprintf(narrative,sizeof(narrative),"%s",out->report);
     snprintf(out->report,sizeof(out->report),
         "Module: %s. Symptom: %s. Source investigation location: %s. "
         "Location evidence: %s. Diagnostic reason: %s. %.260s",
         out->affected_module[0]?out->affected_module:"NOT_IDENTIFIED",
         out->symptom,
         out->source_location[0]?out->source_location:"NOT_ESTABLISHED",
         out->location_status==DIGIT_DSD_LOCATION_INVESTIGATE?
             "INVESTIGATION_TARGET_NOT_VERIFIED_ROOT_CAUSE":"UNRESOLVED",
         out->reason,narrative);
    }
}
static int inspect(const digit_dsd_request_t *in,digit_dsd_result_t *out){
    size_t i;
    unsigned int ok=0;
    if(!in||!out||in->stage_count>DIGIT_DSD_STAGE_COUNT||
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
            if(out->finding==DIGIT_DSD_UNKNOWN){
                out->finding=DIGIT_DSD_OBSERVED;
                snprintf(out->affected_module,sizeof(out->affected_module),"%s",stage->module);
                snprintf(out->reason,sizeof(out->reason),"%s",
                         stage->reason[0]?stage->reason:"STAGE_UNAVAILABLE");
            }
        }
    }
    if(out->finding==DIGIT_DSD_OBSERVED){
        snprintf(out->report,sizeof(out->report),
                 "Observed processing boundary at %s: %s. Retrieved %u record(s), selected %u. This identifies the reported boundary, not a verified underlying root cause. No repair or deployment is authorized.",
                 out->affected_module,out->reason,in->retrieved,in->selected);
    }else if(in->retrieved>0&&in->selected==0){
        out->finding=DIGIT_DSD_INFERRED;
        snprintf(out->affected_module,sizeof(out->affected_module),"response");
        snprintf(out->reason,sizeof(out->reason),"NO_EVIDENCE_SELECTED");
        snprintf(out->report,sizeof(out->report),
                 "Corpus returned %u record(s), but Response selected none. This locates an evidence-selection boundary; the underlying cause is unknown.",
                 in->retrieved);
    }else if(in->selected>0&&!in->candidate_constructed){
        out->finding=DIGIT_DSD_INFERRED;
        snprintf(out->affected_module,sizeof(out->affected_module),"response");
        snprintf(out->reason,sizeof(out->reason),"NO_ANSWER_CANDIDATE");
        snprintf(out->report,sizeof(out->report),
                 "Response selected %u record(s) but no answer candidate was constructed. Inspect the learned relation and composition rules; root cause not established.",
                 in->selected);
    }else if(in->candidate_constructed&&!in->outbound_approved){
        out->finding=DIGIT_DSD_INFERRED;
        snprintf(out->affected_module,sizeof(out->affected_module),"validator");
        snprintf(out->reason,sizeof(out->reason),"CANDIDATE_NOT_APPROVED");
        snprintf(out->report,sizeof(out->report),
                 "A candidate was constructed but not approved. Check the recorded Validator disposition; rejection versus unavailable validation is not established.");
    }else if(in->stage_count>0&&ok==in->stage_count&&in->outbound_approved){
        out->finding=DIGIT_DSD_OBSERVED;
        snprintf(out->reason,sizeof(out->reason),"NO_REPORTED_FAILURE");
        snprintf(out->report,sizeof(out->report),
                 "The supplied stages report success and the outbound candidate was approved. This is not proof of live conversational acceptance.");
    }else{
        out->finding=DIGIT_DSD_UNKNOWN;
        snprintf(out->reason,sizeof(out->reason),"INSUFFICIENT_EVIDENCE");
        snprintf(out->report,sizeof(out->report),
                 "Available telemetry: %u stage(s), %u reporting success, retrieved %u record(s), selected %u. No failed boundary was established. Additional stage telemetry is required to determine the underlying cause.",
                 in->stage_count,ok,in->retrieved,in->selected);
    }
    engineering_handoff(out);
    return 1;
}
static stnlabz_module_result_t diagnose_service(const void *request,size_t request_size,
    void *response,size_t response_size,size_t *response_used,void *context){
    digit_dsd_result_t result;
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
    digit_dsd_result_t answer;
    unsigned int passed=0;
    if(!result)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result,0,sizeof(*result));
    q.retrieved=2;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.finding==DIGIT_DSD_INFERRED&&
        strcmp(answer.reason,"NO_EVIDENCE_SELECTED")==0);
    q.selected=1;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.finding==DIGIT_DSD_INFERRED&&
        strcmp(answer.reason,"NO_ANSWER_CANDIDATE")==0);
    q.candidate_constructed=1;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.finding==DIGIT_DSD_INFERRED&&
        strcmp(answer.reason,"CANDIDATE_NOT_APPROVED")==0);
    q.outbound_approved=1;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.finding==DIGIT_DSD_UNKNOWN);
    q.stage_count=1;
    snprintf(q.stages[0].module,sizeof(q.stages[0].module),"interpretation");
    snprintf(q.stages[0].reason,sizeof(q.stages[0].reason),"SERVICE_TIMEOUT");
    q.stages[0].state=DIGIT_DSD_FAILED;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.finding==DIGIT_DSD_OBSERVED&&
        strcmp(answer.affected_module,"interpretation")==0);
    q.stages[0].state=DIGIT_DSD_OK;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.finding==DIGIT_DSD_OBSERVED&&
        strcmp(answer.reason,"NO_REPORTED_FAILURE")==0);
    q.stage_count=DIGIT_DSD_STAGE_COUNT+1;
    passed+=(unsigned)!inspect(&q,&answer);
    q.stage_count=1;
    q.stages[0].state=DIGIT_DSD_FAILED;
    snprintf(q.stages[0].reason,sizeof(q.stages[0].reason),"INVALID CODE");
    passed+=(unsigned)!inspect(&q,&answer);
    q.stages[0].reason[0]=0;
    passed+=(unsigned)(inspect(&q,&answer)&&answer.finding==DIGIT_DSD_OBSERVED&&
        strcmp(answer.reason,"STAGE_UNAVAILABLE")==0);
    q.stage_count=0;q.retrieved=1;q.selected=2;
    passed+=(unsigned)!inspect(&q,&answer);
    /* Independent regression cases for the new engineering handoff. */
    memset(&q,0,sizeof(q));q.retrieved=2;
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.affected_module,"response")==0&&
      strcmp(answer.source_location,"modules/response/src/response.c")==0&&
      answer.location_status==DIGIT_DSD_LOCATION_INVESTIGATE&&
      strstr(answer.report,"Module: response.")!=NULL);
    memset(&q,0,sizeof(q));q.stage_count=1;
    snprintf(q.stages[0].module,sizeof(q.stages[0].module),"intent");
    snprintf(q.stages[0].reason,sizeof(q.stages[0].reason),"SERVICE_TIMEOUT");
    q.stages[0].state=DIGIT_DSD_FAILED;
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.source_location,"modules/intent/src/intent.c")==0&&
      strstr(answer.symptom,"SERVICE_TIMEOUT")!=NULL);
    snprintf(q.stages[0].module,sizeof(q.stages[0].module),"unmapped");
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.affected_module,"unmapped")==0&&
      !answer.source_location[0]&&
      answer.location_status==DIGIT_DSD_LOCATION_UNRESOLVED);
    memset(&q,0,sizeof(q));
    passed+=(unsigned)(inspect(&q,&answer)&&
      !answer.affected_module[0]&&!answer.source_location[0]&&
      answer.location_status==DIGIT_DSD_LOCATION_UNRESOLVED);
    memset(&q,0,sizeof(q));q.selected=1;q.retrieved=1;
    passed+=(unsigned)(inspect(&q,&answer)&&
      strcmp(answer.source_location,"modules/response/src/response.c")==0&&
      strstr(answer.report,"NOT_VERIFIED_ROOT_CAUSE")!=NULL);
    result->tests_executed=15;
    result->tests_passed=passed;
    result->tests_failed=15-passed;
    result->negative_test_executed=3;
    result->negative_test_passed=passed==15?3:0;
    return passed==15?STNLABZ_MODULE_OK:STNLABZ_MODULE_ERR_QUALIFICATION;
}
static stnlabz_module_result_t dsd_start(const stnlabz_module_host_t *host){
    if(!host||!host->register_service||!host->unregister_service)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!host->register_service(DIGIT_DSD_SERVICE,diagnose_service,NULL))
        return STNLABZ_MODULE_ERR_START_FAILED;
    dsd_host=host;
    if(host->send_message)(void)host->send_message("[DSD] read-only diagnostic service active");
    return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t dsd_stop(void){
    int ok=1;
    if(dsd_host&&dsd_host->unregister_service)
        ok=dsd_host->unregister_service(DIGIT_DSD_SERVICE,NULL);
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
