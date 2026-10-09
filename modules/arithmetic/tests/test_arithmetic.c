#include <stdio.h>
#include <string.h>
#include "arithmetic.h"

/* [AI:GPT-6 | 2026-10-09] Exercise public service ABI with a mock Core host. */
static stnlabz_module_service_handler_fn handler;
static int reg(const char *name,stnlabz_module_service_handler_fn fn,void *context){
 (void)context;if(strcmp(name,DIGIT_ARITHMETIC_SERVICE))return 0;handler=fn;return 1;
}
static int unreg(const char *name,void *context){
 (void)context;return strcmp(name,DIGIT_ARITHMETIC_SERVICE)==0?1:0;
}
static unsigned int checks,failures;
static void check(int condition,const char *name){
 ++checks;if(!condition){++failures;printf("FAIL %u %s\n",checks,name);}
 else printf("PASS %u %s\n",checks,name);
}
int main(void){
 const stnlabz_module_descriptor_t *d=stnlabz_module_get_descriptor();
 stnlabz_module_qualification_result_t q;
 stnlabz_module_host_t host={0};
 digit_arithmetic_request_t in={0};
 digit_arithmetic_result_t out={0};size_t used=0;
 check(d&&strcmp(d->id,"arithmetic")==0,"descriptor identity");
 check(d&&d->version_major==1&&d->version_minor==3&&d->version_patch==0,"version 1.3.0");
 check(d&&d->qualify(&q)==STNLABZ_MODULE_OK,"qualification executes");
 check(q.tests_executed>=STNLABZ_MODULE_MIN_TESTS&&q.tests_passed==q.tests_executed&&q.tests_failed==0,"qualification counts");
 check(q.negative_test_executed&&q.negative_test_passed,"negative qualification");
 host.register_service=reg;host.unregister_service=unreg;
 check(d->start(&host)==STNLABZ_MODULE_OK&&handler,"service registration");
 snprintf(in.expression,sizeof(in.expression),"Digit what is 8 divided by 2?");
 check(handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_OK &&
       used==sizeof(out)&&out.status==DIGIT_ARITHMETIC_OK&&out.value==4,"service computes four");
 memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));used=0;
 snprintf(in.expression,sizeof(in.expression),"7 divided by zero");
 check(handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_OK &&
       out.status==DIGIT_ARITHMETIC_DIVIDE_BY_ZERO,"service rejects zero denominator");
 /* [AI:GPT-6 | 2026-10-09] Decimal service ABI and fixed-point boundaries. */
 {
  static const struct {const char *input,*expected;digit_arithmetic_status_t status;} cases[]={
   {"Digit what is 1000 / 0.5?","1000 / 0.5 = 2000.",DIGIT_ARITHMETIC_OK},
   {"Digit what is 2.5 + 3.75?","2.5 + 3.75 = 6.25.",DIGIT_ARITHMETIC_OK},
   {"Digit what is 2.5 * 2?","2.5 * 2 = 5.",DIGIT_ARITHMETIC_OK},
   {"Digit what is 1 / 0.5?","1 / 0.5 = 2.",DIGIT_ARITHMETIC_OK},
   {"Digit what is 1 / 0.0?","",DIGIT_ARITHMETIC_DIVIDE_BY_ZERO},
   {"Digit what is 1 / 0.0001?","",DIGIT_ARITHMETIC_OUT_OF_RANGE},
   {"Digit what is 2.5 + 1 + 1?","Result = 4.5.",DIGIT_ARITHMETIC_OK}
  };
  size_t i;
  for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
   memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));used=0;
   snprintf(in.expression,sizeof(in.expression),"%s",cases[i].input);
   check(handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_OK&&
    used==sizeof(out)&&out.status==cases[i].status&&
    (out.status!=DIGIT_ARITHMETIC_OK||strcmp(out.decimal_answer,cases[i].expected)==0),
    cases[i].input);
  }
 }
 /* [AI:GPT-6 | 2026-10-09] Public-service regression: precedence,
  * grouping, linear equations, and unsupported nonlinear equations. */
 {
  static const struct {const char *input,*answer;digit_arithmetic_status_t status;} algebra[]={
   {"2 + 3 * 4","Result = 14.",DIGIT_ARITHMETIC_OK},
   {"(2 + 3) * 4","Result = 20.",DIGIT_ARITHMETIC_OK},
   {"-(2 + 3) * 4","Result = -20.",DIGIT_ARITHMETIC_OK},
   {"2*x+3=11","x = 4.",DIGIT_ARITHMETIC_OK},
   {"solve 3*x - 6 = 0","x = 2.",DIGIT_ARITHMETIC_OK},
   {"x + 1 = x + 1","Infinitely many solutions.",DIGIT_ARITHMETIC_OK},
   {"x + 1 = x + 2","No solution.",DIGIT_ARITHMETIC_OK},
   {"x*x = 4","",DIGIT_ARITHMETIC_INVALID},
   {"x / x = 1","",DIGIT_ARITHMETIC_INVALID},
   {"(2 + 3","",DIGIT_ARITHMETIC_INVALID},
   {"(1 / 0) + 4","",DIGIT_ARITHMETIC_DIVIDE_BY_ZERO}
  };
  size_t i;
  for(i=0;i<sizeof(algebra)/sizeof(algebra[0]);++i){
   memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));used=0;
   snprintf(in.expression,sizeof(in.expression),"%s",algebra[i].input);
   check(handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_OK &&
    used==sizeof(out)&&out.status==algebra[i].status &&
    (out.status!=DIGIT_ARITHMETIC_OK||strcmp(out.decimal_answer,algebra[i].answer)==0),
    algebra[i].input);
  }
 }
 /* [AI:GPT-6 | 2026-10-09] Quadratic solver regression coverage. */
 {
  static const struct {const char *input,*expected;} equations[]={
   {"solve x^2 - 5*x + 6 = 0","x = 2 and x = 3."},
   {"solve x^2 - 4*x + 4 = 0","x = 2."},
   {"solve x^2 + 1 = 0","No real solutions."},
   {"solve (x+1)*(x+2) = 0","x = -2 and x = -1."},
   {"solve x^2 - 2 = 0","Approximately: x = -1.414 and x = 1.414."}
  };
  size_t i;
  for(i=0;i<sizeof(equations)/sizeof(equations[0]);++i){
   memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));used=0;
   snprintf(in.expression,sizeof(in.expression),"%s",equations[i].input);
   check(handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_OK&&
     used==sizeof(out)&&out.status==DIGIT_ARITHMETIC_OK&&
     strcmp(out.decimal_answer,equations[i].expected)==0,equations[i].input);
  }
 }
 memset(&in,'x',sizeof(in));
 check(handler(&in,sizeof(in),&out,sizeof(out),&used,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT,
       "service rejects unterminated expression");
 check(d->stop()==STNLABZ_MODULE_OK,"service unregisters");
 printf("Arithmetic: %u checks, %u failures\n",checks,failures);
 return failures?1:0;
}
