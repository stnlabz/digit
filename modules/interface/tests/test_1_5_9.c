#include <stdio.h>
#include "sa_channel_policy.h"
/* [AI:GPT-6 | 2026-10-09] Permanent SA namespace policy and
 * negative cross-organization qualification. */
static int executed,failed;
static void check(int ok,const char *label){++executed;
    if(ok)printf("PASS %02d - %s\n",executed,label);
    else{++failed;printf("FAIL %02d - %s\n",executed,label);}
}
int main(void)
{
    check(digit_sa_channel_permanent("Security"),"Security permanent");
    check(digit_sa_channel_permanent("Alerts"),"Alerts permanent");
    check(!digit_sa_channel_permanent("general"),"ordinary channel not permanent");
    check(!digit_sa_channel_permanent("alerts"),"case-sensitive channel identity");
    check(!digit_sa_channel_permanent(NULL),"null channel denied");
    check(digit_sa_channel_allowed("stn-labz","stn-labz",1,1,1,"Alerts"),"STN-Labz SA Alerts");
    check(digit_sa_channel_allowed("team-chaos","team-chaos",1,1,1,"Security"),"Team ChAoS SA Security");
    check(!digit_sa_channel_allowed("stn-labz","team-chaos",1,1,1,"Alerts"),"Team ChAoS denied STN-Labz");
    check(!digit_sa_channel_allowed("team-chaos","stn-labz",1,1,1,"Security"),"STN-Labz denied Team ChAoS");
    check(!digit_sa_channel_allowed("stn-labz","stn-labz",0,1,1,"Alerts"),"unqualified denied");
    check(!digit_sa_channel_allowed("stn-labz","stn-labz",1,0,1,"Alerts"),"unassigned denied");
    check(!digit_sa_channel_allowed("stn-labz","stn-labz",1,1,0,"Alerts"),"mission-unqualified denied");
    check(!digit_sa_channel_allowed("stn-labz","stn-labz",1,1,1,"general"),"ordinary channel outside SA policy");
    check(!digit_sa_channel_allowed(NULL,"stn-labz",1,1,1,"Alerts"),"null request org denied");
    check(!digit_sa_channel_allowed("stn-labz",NULL,1,1,1,"Alerts"),"null assignment org denied");
    check(!digit_sa_channel_allowed("","stn-labz",1,1,1,"Alerts"),"empty org denied");
    check(!digit_sa_channel_allowed("stn-labz","stn-labz",2,1,1,"Alerts"),"noncanonical qualification denied");
    check(!digit_sa_channel_allowed("stn-labz","stn-labz",1,1,1,NULL),"null channel denied");
    printf("Interface 1.5.9 policy milestone: %d executed, %d failed\n",executed,failed);
    return failed?1:0;
}
