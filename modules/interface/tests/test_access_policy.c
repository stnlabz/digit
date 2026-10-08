#include <stdio.h>
#include <string.h>
#include "access_policy.h"

/* [AI:GPT-6 | 2026-10-08] Identity/ACL negative qualification tests. */
static unsigned int run = 0, failed = 0;
static void check(int pass, const char *name)
{
    ++run;
    if (pass) printf("PASS %02u - %s\n", run, name);
    else { ++failed; printf("FAIL %02u - %s\n", run, name); }
}

int main(void)
{
    digit_access_principal_t p = {"ezra", "stn-labz", 1, 1, 1, 1, 1};
    digit_access_resource_t r = {"stn-labz", "digit", "development", "ezra", 1};

    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_ELIGIBLE,"all three requirements with grant and authenticated identity");
    check(digit_access_evaluate(NULL,&r)==DIGIT_ACCESS_DENIED,"null principal denied");
    check(digit_access_evaluate(&p,NULL)==DIGIT_ACCESS_DENIED,"null resource denied");
    p.authenticated=0;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"unauthenticated denied");
    p.authenticated=1;p.account_active=0;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"disabled account denied");
    p.account_active=1;p.qualification_valid=0;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"missing qualification denied");
    p.qualification_valid=1;p.job_assignment_active=0;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"missing job assignment denied");
    p.job_assignment_active=1;p.mission_qualification_valid=0;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"missing mission qualification denied");
    p.mission_qualification_valid=1;r.acl_grant_active=0;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"revoked resource grant denied");
    r.acl_grant_active=1;r.authorized_user_id="poe";
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"grant for another user denied");
    r.authorized_user_id="ezra";r.organization_id="chaos";
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"cross-organization denied");
    r.organization_id="stn-labz";r.project_id=NULL;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"missing project scope denied");
    r.project_id="digit";r.channel_id="";
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"missing channel scope denied");
    r.channel_id="development";p.user_id="";
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"missing user identity denied");
    p.user_id="ezra";p.organization_id=NULL;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"missing principal organization denied");
    p.organization_id="stn-labz";p.qualification_valid=2;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_DENIED,"noncanonical validity value denied");
    p.qualification_valid=1;
    check(digit_access_evaluate(&p,&r)==DIGIT_ACCESS_ELIGIBLE,"restored complete eligibility succeeds");
    printf("\nAccess policy tests: %u executed, %u failed\n",run,failed);
    return failed?1:0;
}
