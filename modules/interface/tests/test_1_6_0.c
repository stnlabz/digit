#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"
#include "channel_acl.h"
#include "grant_request.h"
#include "grant_store.h"
#include "alerts_channel.h"
/* [AI:GPT-6 | 2026-10-09] 1.6.0 positive and negative input tests. */
static unsigned tests,failures;
static void check(int valid,const char *label){
 ++tests;
 if(valid)printf("PASS %02u - %s\n",tests,label);
 else{++failures;printf("FAIL %02u - %s\n",tests,label);}
}
int main(void){
 digit_grant_request_t r;
 char long_name[200];
 char base[]="/tmp/digit-grants-XXXXXX",projects[256],auth[256],roster[300],grants[300],binding[400];
 FILE *file;
 memset(long_name,'x',sizeof(long_name)-1);
 long_name[sizeof(long_name)-1]=0;
 check(digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei",&r),"valid grant parsed");
 check(strcmp(r.organization,"stn-labz")==0,"organization intact");
 check(strcmp(r.project,"operations")==0,"project intact");
 check(strcmp(r.channel,"channel-01")==0,"channel intact");
 check(strcmp(r.user,"poemei")==0,"operator intact");
 check(digit_grant_request_parse("team-chaos\tsecurity\tchannel-02\tpoemei",&r),"separate organization syntax");
 check(!digit_grant_request_parse(NULL,&r),"null input denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei",NULL),"null output denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01",&r),"missing user denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei\textra",&r),"extra field denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\t",&r),"empty user denied");
 check(!digit_grant_request_parse("\toperations\tchannel-01\tpoemei",&r),"empty org denied");
 check(!digit_grant_request_parse("../other\toperations\tchannel-01\tpoemei",&r),"traversal denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei\n",&r),"newline denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tbad user",&r),"spaces denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tbad\ruser",&r),"carriage return denied");
 check(!digit_grant_request_parse(long_name,&r),"oversize denied");
 check(!digit_grant_request_parse("stn-labz\toperations\tchannel-01\tpoemei\t",&r),"trailing tab denied");
 check(!digit_grant_security_scoped(NULL,"/tmp","/tmp","poemei",&r),"missing registry denied");
 check(!digit_grant_security_scoped("/tmp/channel_grants.tsv",NULL,"/tmp","poemei",&r),"missing project root denied");
 check(!digit_grant_security_scoped("/tmp/channel_grants.tsv","/tmp",NULL,"poemei",&r),"missing SA roster denied");
 check(!digit_grant_security_scoped("/tmp/channel_grants.tsv","/tmp","/tmp",NULL,&r),"missing actor denied");
 check(!digit_grant_security_scoped("/tmp/channel_grants.tsv","/tmp","/tmp","poemei",NULL),"missing grant denied");
 check(!digit_grant_security_scoped("/tmp/channel_grants.tsv","/tmp","/tmp","poemei",&r),"unverified actor and target denied");
 /* [AI:GPT-6 | 2026-10-09] Isolated positive persistence and
  * cross-organization negative cases; no production file access. */
 check(mkdtemp(base)!=NULL,"isolated grant fixture created");
 check(chmod(base,0700)==0,"fixture directory private");
 snprintf(projects,sizeof(projects),"%s/projects",base);
 snprintf(auth,sizeof(auth),"%s/auth",base);
 check(mkdir(projects,0700)==0,"projects root private");
 check(mkdir(auth,0700)==0,"authorization root private");
 snprintf(roster,sizeof(roster),"%s/security_sa.tsv",auth);
 snprintf(grants,sizeof(grants),"%s/channel_grants.tsv",auth);
 file=fopen(roster,"w");
 check(file!=NULL,"SA roster fixture opened");
 if(file){fputs("poemei\tstn-labz\tSA\t1\t1\t1\n",file);fclose(file);}
 check(chmod(roster,0600)==0,"SA roster private");
 check(digit_project_provision(projects,"stn-labz","operations","poemei","poemei",roster),
       "project provisioned with verified SA");
 snprintf(binding,sizeof(binding),"%s/stn-labz/operations/security_channel.id",projects);
 file=fopen(binding,"w");
 check(file!=NULL,"Security binding fixture opened");
 if(file){fputs("security-01\n",file);fclose(file);}
 check(chmod(binding,0600)==0,"Security binding private");
 check(!digit_alerts_channel_lookup(projects,"stn-labz","operations",binding,sizeof(binding)),
       "missing Alerts binding denied");
 check(!digit_alerts_channel_ensure(projects,"stn-labz","operations","poemei",roster,
       NULL,binding,sizeof(binding)),"missing Core host denies Alerts creation");
 check(digit_grant_request_parse("stn-labz\toperations\talerts-01\tpoemei",&r),
       "Alerts grant syntax accepted");
 check(!digit_grant_alerts_scoped(grants,projects,roster,"poemei",&r),
       "unbound Alerts grant denied");
 check(digit_grant_request_parse("stn-labz\toperations\tsecurity-01\tpoemei",&r),
       "Security grant request parsed");
 check(digit_grant_security_scoped(grants,projects,roster,"poemei",&r),
       "authorized Security grant atomically created");
 check(digit_channel_acl_check_scoped(grants,projects,roster,"poemei","security-01"),
       "created grant is readable by enforcement ACL");
 check(!digit_grant_security_scoped(grants,projects,roster,"poemei",&r),
       "duplicate operator/channel denied");
 check(digit_grant_request_parse("team-chaos\toperations\tsecurity-01\tpoemei",&r),
       "cross-organization request parsed");
 check(!digit_grant_security_scoped(grants,projects,roster,"poemei",&r),
       "unassigned organization denied");
 check(digit_grant_request_parse("stn-labz\toperations\tsecurity-02\tpoemei",&r),
       "unbound channel parsed");
 check(!digit_grant_security_scoped(grants,projects,roster,"poemei",&r),
       "unbound channel denied");
 check(digit_channel_acl_check_scoped(grants,projects,roster,"poemei","security-01"),
       "rejected requests preserve existing grant");
 unlink(grants);unlink(roster);unlink(binding);
 {
  char p[400];
  snprintf(p,sizeof(p),"%s/stn-labz/operations/READY",projects);unlink(p);
  snprintf(p,sizeof(p),"%s/stn-labz/operations/security.tsv",projects);unlink(p);
  snprintf(p,sizeof(p),"%s/stn-labz/operations/project.tsv",projects);unlink(p);
  snprintf(p,sizeof(p),"%s/stn-labz/operations",projects);rmdir(p);
  snprintf(p,sizeof(p),"%s/stn-labz",projects);rmdir(p);
 }
 rmdir(projects);rmdir(auth);rmdir(base);
 printf("Interface 1.6.0 scope parser: %u tests, %u failed\n",tests,failures);
 return failures?1:0;
}
