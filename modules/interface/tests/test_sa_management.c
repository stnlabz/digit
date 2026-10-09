#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sa_management.h"
#include "security_sa.h"
/* [AI:GPT-6 | 2026-10-09] 1.6.5 SA management qualification. */
static int passed=0,failed=0;
static void check(int cond,const char *what){printf("%s %s\n",cond?"PASS":"FAIL",what);if(cond)passed++;else failed++;}
static int write_roster(const char *path){
 FILE *f=fopen(path,"wb");
 if(!f)return 0;
 if(fputs("poemei\tstn-labz\tSA\t1\t1\t1\n"
          "assistant\tstn-labz\tADMIN\t1\t0\t1\n"
          "trainee\tstn-labz\tADMIN\t0\t0\t0\n"
          "chief\tteam-chaos\tSA\t1\t1\t1\n"
          "poemei\tteam-chaos\tADMIN\t1\t0\t1\n",f)<0){fclose(f);return 0;}
 if(fclose(f)!=0)return 0;
 return chmod(path,0600)==0;
}
int main(void){
 char dir[]="/tmp/digit-sa-test-XXXXXX",file[512],json[8192];
 if(!mkdtemp(dir))return 1;
 snprintf(file,sizeof(file),"%s/security_sa.tsv",dir);
 if(!write_roster(file)){rmdir(dir);return 1;}
 check(digit_sa_list(file,"stn-labz","poemei",json,sizeof(json)),"authorized list");
 check(strstr(json,"\"user\":\"assistant\"")!=NULL,"list includes org member");
 check(strstr(json,"\"user\":\"chief\"")==NULL,"list excludes other org");
 check(!digit_sa_list(file,"team-chaos","poemei",json,sizeof(json)),"no cross-org listing");
 check(!digit_sa_change(file,"team-chaos","poemei","poemei",1),"no cross-org elevation");
 check(!digit_sa_change(file,"stn-labz","assistant","assistant",1),"non-SA denied");
 check(!digit_sa_change(file,"stn-labz","poemei","trainee",1),"unqualified denied");
 check(!digit_sa_change(file,"stn-labz","poemei","unknown",1),"unknown target denied");
 check(digit_sa_change(file,"stn-labz","poemei","assistant",1),"qualified assignment");
 check(digit_security_sa_verify(file,"stn-labz","assistant"),"assigned SA verifies");
 check(!digit_sa_change(file,"stn-labz","poemei","poemei",0),"self revoke denied");
 check(digit_sa_change(file,"stn-labz","poemei","assistant",0),"revocation");
 check(!digit_security_sa_verify(file,"stn-labz","assistant"),"revocation immediate");
 check(digit_security_sa_verify(file,"team-chaos","chief"),"unrelated org unchanged");
 check(!digit_sa_change(file,"team-chaos","poemei","poemei",2),"bootstrap denied when Team ChAoS has SA");
 {
  FILE *b=fopen(file,"wb");
  if(!b)return 1;
  if(fputs("poemei\tstn-labz\tSA\t1\t1\t1\n"
           "poemei\tteam-chaos\tADMIN\t1\t0\t1\n",b)<0){fclose(b);return 1;}
  fclose(b);if(chmod(file,0600)!=0)return 1;
 }
 check(!digit_sa_change(file,"team-chaos","other","poemei",2),"bootstrap only authenticated founder identity");
 check(!digit_sa_change(file,"stn-labz","poemei","poemei",2),"bootstrap restricted target organization");
 check(digit_sa_change(file,"team-chaos","poemei","poemei",2),"founder bootstraps qualified initial Team ChAoS SA");
 check(digit_security_sa_verify(file,"team-chaos","poemei"),"Team ChAoS SA active after bootstrap");
 check(!digit_sa_change(file,"team-chaos","poemei","poemei",2),"bootstrap nonrepeatable");
 unlink(file);rmdir(dir);
 printf("SA 1.6.5: %d passed %d failed\n",passed,failed);
 return failed!=0;
}
