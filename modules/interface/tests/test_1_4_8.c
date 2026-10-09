#include <stdio.h>
#include <string.h>
#include "alert_results.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.8: positive and
 * negative alert-list service-boundary regression checks. */
static unsigned int count,failed;
static void check(int good,const char *name)
{
    ++count;
    printf("%s 1.4.8 %02u - %s\n",good?"PASS":"FAIL",count,name);
    if(!good)++failed;
}
static digit_alert_t example(void)
{
    digit_alert_t a={0};
    strcpy(a.id,"alert-1");
    strcpy(a.source,"sentinel");
    strcpy(a.summary,"Event detected");
    strcpy(a.detail,"System observation");
    strcpy(a.operational_state,"NOMINAL");
    a.severity=DIGIT_ALERT_INFO;
    return a;
}
int main(void)
{
    digit_alert_t a[2]={0};
    a[0]=example();a[1]=example();
    strcpy(a[1].id,"alert-2");
    check(digit_interface_alerts_valid(a,2,2,0),"two valid alerts accepted");
    check(digit_interface_alerts_valid(NULL,0,2,0),"empty list accepted");
    check(!digit_interface_alerts_valid(NULL,1,2,0),"missing nonempty list rejected");
    check(!digit_interface_alerts_valid(a,3,2,0),"oversized count rejected before indexing");
    check(!digit_interface_alerts_valid(a,1,0,0),"zero capacity with record rejected");
    check(!digit_interface_alerts_valid(a,0,2,2),"invalid requested filter rejected");
    a[1].severity=(digit_alert_severity_t)99;
    check(!digit_interface_alerts_valid(a,2,2,0),"invalid alert severity rejected");
    a[1]=example();strcpy(a[1].id,"alert-2");
    a[1].acknowledged=2;
    check(!digit_interface_alerts_valid(a,2,2,0),"invalid acknowledgement state rejected");
    a[1].acknowledged=1;
    check(!digit_interface_alerts_valid(a,2,2,1),"acknowledged alert denied from unacknowledged list");
    check(digit_interface_alerts_valid(a,2,2,0),"acknowledged alert accepted in full list");
    a[1]=example();strcpy(a[1].id,"alert-2");
    check(digit_interface_alerts_valid(a,2,2,1),"unacknowledged entries accepted under filter");
    strcpy(a[1].id,"alert-1");
    check(!digit_interface_alerts_valid(a,2,2,0),"duplicate alert identity denied");
    strcpy(a[1].id,"alert/2");
    check(!digit_interface_alerts_valid(a,2,2,0),"unsafe alert identifier denied");
    strcpy(a[1].id,"alert-2");
    a[1].source[0]=0;
    check(!digit_interface_alerts_valid(a,2,2,0),"missing source denied");
    strcpy(a[1].source,"sentinel");
    a[1].summary[0]=0;
    check(!digit_interface_alerts_valid(a,2,2,0),"missing summary denied");
    strcpy(a[1].summary,"Event detected");
    a[1].operational_state[0]=0;
    check(!digit_interface_alerts_valid(a,2,2,0),"missing operational state denied");
    strcpy(a[1].operational_state,"NOMINAL");
    memset(a[1].detail,'x',sizeof(a[1].detail));
    check(!digit_interface_alerts_valid(a,2,2,0),"unterminated detail denied");
    strcpy(a[1].detail,"System observation");
    a[1].summary[0]='\n';
    check(!digit_interface_alerts_valid(a,2,2,0),"embedded control character denied");
    strcpy(a[1].summary,"Event detected");
    check(digit_interface_alerts_valid(a,2,2,0),"validation recovers after bad records");
    printf("Interface 1.4.8 milestone: %u executed, %u failed\n",count,failed);
    return failed!=0;
}
