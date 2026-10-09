#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "security_sa.h"
#include "project_provision.h"
#include "channel_acl.h"
/* [AI:GPT-6 | 2026-10-08] SA and membership lifecycle re-evaluation. */
static int total,fail;
static void check(int ok,const char *s){++total;printf("%s 1.3.7 %02d - %s\n",ok?"PASS":"FAIL",total,s);if(!ok)++fail;}
static int put(const char *p,const char *s){FILE *f=fopen(p,"w");int ok;if(!f)return 0;ok=fputs(s,f)>=0;if(fclose(f))ok=0;return ok&&chmod(p,0600)==0;}
int main(void){
 char root[]="/tmp/digit-137-XXXXXX",projects[256],dir[320],sa[256],acl[256],member[384],bound[384];
 if(!mkdtemp(root))return 1;
 /* [AI:GPT-6 | 2026-10-08] Keep SA/ACL registries outside the
  * strictly directory-only project inventory; mixed files deny scanning. */
 snprintf(projects,sizeof(projects),"%s/projects",root);
 if(mkdir(projects,0700)!=0)return 1;
 snprintf(dir,sizeof(dir),"%s/stn-labz/digit",projects);
 snprintf(sa,sizeof(sa),"%s/sa.tsv",root);
 snprintf(acl,sizeof(acl),"%s/acl.tsv",root);
 snprintf(member,sizeof(member),"%s/security.tsv",dir);
 snprintf(bound,sizeof(bound),"%s/security_channel.id",dir);
 check(put(sa,"admin\tstn-labz\tSA\t1\t1\t1\n"),"SA fixture");
 check(digit_project_provision(projects,"stn-labz","digit","poe","admin",sa),"project fixture");
 check(put(bound,"channel137\n"),"binding fixture");
 check(put(acl,"admin\tstn-labz\tdigit\tchannel137\t1\t1\t1\t1\n"),"grant fixture");
 check(digit_channel_acl_check_scoped(acl,projects,sa,"admin","channel137"),"initially authorized");
 check(put(sa,"admin\tstn-labz\tSA\t1\t0\t1\n"),"revoke assignment");
 check(!digit_channel_acl_check_scoped(acl,projects,sa,"admin","channel137"),"assignment revoked next request");
 check(put(sa,"admin\tstn-labz\tSA\t1\t1\t1\n"),"restore assignment");
 check(digit_channel_acl_check_scoped(acl,projects,sa,"admin","channel137"),"restored assignment active");
 check(put(member,"security\trestricted\tdigit\n"),"revoke membership");
 check(!digit_channel_acl_check_scoped(acl,projects,sa,"admin","channel137"),"membership revoked next request");
 check(put(member,"security\trestricted\tdigit\nsecurity\trestricted\tadmin\n"),"restore membership");
 check(digit_channel_acl_check_scoped(acl,projects,sa,"admin","channel137"),"restored membership active");
 check(put(sa,"admin\tstn-labz\tADMIN\t1\t1\t1\n"),"downgrade SA role");
 check(!digit_channel_acl_check_scoped(acl,projects,sa,"admin","channel137"),"downgrade denies next request");
 printf("Interface 1.3.7 milestone: %d executed, %d failed\n",total,fail);
 return fail!=0;
}
