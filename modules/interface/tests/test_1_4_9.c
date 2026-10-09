#include <stdio.h>
#include <string.h>
#include "alert_results.h"
/* [AI:GPT-6 | 2026-10-08] 1.4.9 exact response tests. */
static unsigned int executed,failed;
static void check(int ok,const char *why){++executed;printf("%s 1.4.9 %02u - %s\n",ok?"PASS":"FAIL",executed,why);if(!ok)++failed;}
int main(void){
 digit_alert_t a={0};
 strcpy(a.id,"alert-001");strcpy(a.source,"digit");
 strcpy(a.summary,"Observation");strcpy(a.detail,"detail");
 strcpy(a.operational_state,"NOMINAL");a.severity=DIGIT_ALERT_WARNING;
 check(digit_interface_alert_exact_valid(&a,1,"alert-001",0),"matching record accepted");
 check(digit_interface_alert_exact_valid(NULL,0,"alert-001",0),"missing record accepted");
 check(!digit_interface_alert_exact_valid(&a,2,"alert-001",0),"nonboolean found rejected");
 check(!digit_interface_alert_exact_valid(&a,-1,"alert-001",0),"negative found rejected");
 check(!digit_interface_alert_exact_valid(&a,1,"other",0),"identity mismatch rejected");
 check(!digit_interface_alert_exact_valid(NULL,1,"alert-001",0),"null found payload rejected");
 check(!digit_interface_alert_exact_valid(&a,1,"../bad",0),"unsafe requested ID rejected");
 check(!digit_interface_alert_exact_valid(&a,1,"",0),"empty requested ID rejected");
 check(!digit_interface_alert_exact_valid(&a,1,NULL,0),"null requested ID rejected");
 check(!digit_interface_alert_exact_valid(&a,1,"alert-001",2),"invalid ack policy rejected");
 check(!digit_interface_alert_exact_valid(&a,1,"alert-001",1),"unacknowledged success rejected");
 a.acknowledged=1;
 check(digit_interface_alert_exact_valid(&a,1,"alert-001",1),"acknowledgement accepted");
 check(digit_interface_alert_exact_valid(NULL,0,"alert-001",1),"acknowledgement absent accepted");
 a.acknowledged=2;
 check(!digit_interface_alert_exact_valid(&a,1,"alert-001",1),"invalid ack state rejected");
 a.acknowledged=1;a.severity=(digit_alert_severity_t)99;
 check(!digit_interface_alert_exact_valid(&a,1,"alert-001",0),"invalid severity rejected");
 a.severity=DIGIT_ALERT_INFO;a.source[0]=0;
 check(!digit_interface_alert_exact_valid(&a,1,"alert-001",1),"missing source rejected");
 strcpy(a.source,"digit");memset(a.summary,'x',sizeof(a.summary));
 check(!digit_interface_alert_exact_valid(&a,1,"alert-001",1),"unterminated summary rejected");
 strcpy(a.summary,"Observation");
 check(digit_interface_alert_exact_valid(&a,1,"alert-001",1),"recovers after malformed record");
 printf("Interface 1.4.9 milestone: %u executed, %u failed\n",executed,failed);
 return failed!=0;
}
