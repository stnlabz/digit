#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "arithmetic.h"

/* [AI:GPT-6 | 2026-10-09] Independent, deterministic arithmetic service.
 * Shared through Core service registration, never by direct module linkage. */
static const stnlabz_module_host_t *arithmetic_host;

static void skip_spaces(const char **p){while(isspace((unsigned char)**p))++*p;}
static int operand(const char **cursor,int64_t *out){
 static const char *const words[]={"zero","one","two","three","four","five","six","seven","eight","nine","ten"};
 const char *p=*cursor;int64_t number=0;int sign=1;size_t i;
 skip_spaces(&p);
 if(*p=='+'||*p=='-'){if(*p=='-')sign=-1;++p;}
 if(isdigit((unsigned char)*p)){
  do{
   int digit=*p-'0';
   if(number>100000000 || (number==100000000 && digit>0))return 0;
   number=number*10+digit;++p;
  }while(isdigit((unsigned char)*p));
 }else{
  for(i=0;i<sizeof(words)/sizeof(words[0]);++i){
   size_t n=strlen(words[i]);
   if(strncasecmp(p,words[i],n)==0&&!isalnum((unsigned char)p[n])&&p[n]!='_'){
    number=(int64_t)i;p+=n;break;
   }
  }
  if(i==sizeof(words)/sizeof(words[0]))return 0;
 }
 *out=number*sign;*cursor=p;return 1;
}
static int operator_at(const char **cursor,char *out){
 static const struct {const char *name;char op;} words[]={
  {"multiplied by",'*'},{"multiply by",'*'},{"divided by",'/'},{"divide by",'/'},
  {"plus",'+'},{"minus",'-'},{"times",'*'},{"over",'/'},{"x",'*'}
 };
 const char *p=*cursor;size_t i;
 skip_spaces(&p);
 if(*p=='+'||*p=='-'||*p=='*'||*p=='/'){*out=*p++;*cursor=p;return 1;}
 if((unsigned char)p[0]==0xc3 && (unsigned char)p[1]==0x97){*out='*';*cursor=p+2;return 1;}
 if((unsigned char)p[0]==0xc3 && (unsigned char)p[1]==0xb7){*out='/';*cursor=p+2;return 1;}
 for(i=0;i<sizeof(words)/sizeof(words[0]);++i){
  size_t n=strlen(words[i].name);
  if(strncasecmp(p,words[i].name,n)==0&&!isalnum((unsigned char)p[n])&&p[n]!='_'){
   *out=words[i].op;*cursor=p+n;return 1;
  }
 }
 return 0;
}
static digit_arithmetic_status_t evaluate(const char *expression,digit_arithmetic_result_t *result){
 const char *p=expression;int64_t a,b;
 memset(result,0,sizeof(*result));
 if(!expression||!*expression)return DIGIT_ARITHMETIC_INVALID;
 skip_spaces(&p);
 if(strncasecmp(p,"digit",5)==0 && (isspace((unsigned char)p[5])||p[5]==','||p[5]==':')){
  p+=5;if(*p==','||*p==':')++p;skip_spaces(&p);
 }
 if(strncasecmp(p,"what is ",8)==0)p+=8;
 else if(strncasecmp(p,"calculate ",10)==0)p+=10;
 else if(strncasecmp(p,"compute ",8)==0)p+=8;
 if(!operand(&p,&a))return DIGIT_ARITHMETIC_OUT_OF_RANGE;
 if(!operator_at(&p,&result->operation))return DIGIT_ARITHMETIC_INVALID;
 if(!operand(&p,&b))return DIGIT_ARITHMETIC_OUT_OF_RANGE;
 skip_spaces(&p);if(*p=='?')++p;skip_spaces(&p);
 if(*p) return DIGIT_ARITHMETIC_INVALID;
 result->left=a;result->right=b;
 if(result->operation=='/'&&b==0)return DIGIT_ARITHMETIC_DIVIDE_BY_ZERO;
 switch(result->operation){
 case '+':result->value=a+b;break;
 case '-':result->value=a-b;break;
 case '*':result->value=a*b;break;
 case '/':result->value=a/b;result->remainder=a%b;break;
 default:return DIGIT_ARITHMETIC_INVALID;
 }
 return DIGIT_ARITHMETIC_OK;
}
static stnlabz_module_result_t arithmetic_service(const void *request,size_t request_size,
 void *response,size_t response_size,size_t *used,void *context){
 const digit_arithmetic_request_t *in=request;digit_arithmetic_result_t result;
 (void)context;
 if(!request||request_size!=sizeof(*in)||!response||response_size<sizeof(result)||!used)
  return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 if(!memchr(in->expression,0,sizeof(in->expression))||!in->expression[0])
  return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 result.status=evaluate(in->expression,&result);
 memcpy(response,&result,sizeof(result));*used=sizeof(result);
 return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t arithmetic_qualify(stnlabz_module_qualification_result_t *qualification){
 static const struct {const char *input;digit_arithmetic_status_t status;int64_t value;} cases[]={
  {"Digit what is 2+2?",DIGIT_ARITHMETIC_OK,4},
  {"Digit what is two plus two?",DIGIT_ARITHMETIC_OK,4},
  {"Digit what is 2 X 2?",DIGIT_ARITHMETIC_OK,4},
  {"Digit what is 2 × 2?",DIGIT_ARITHMETIC_OK,4},
  {"Digit what is 8 ÷ 2?",DIGIT_ARITHMETIC_OK,4},
  {"8 divided by zero",DIGIT_ARITHMETIC_DIVIDE_BY_ZERO,0},
  {"7 / 2",DIGIT_ARITHMETIC_OK,3},
  {"7 minus 12",DIGIT_ARITHMETIC_OK,-5},
  {"-9 * -3",DIGIT_ARITHMETIC_OK,27},
  {"1000000000 * 1000000000",DIGIT_ARITHMETIC_OK,1000000000000000000LL},
  {"1000000001 + 2",DIGIT_ARITHMETIC_OUT_OF_RANGE,0},
  {"2 plus 2 plus 2",DIGIT_ARITHMETIC_INVALID,0}
 };
 unsigned int passed=0;size_t i;digit_arithmetic_result_t r;
 if(!qualification)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 memset(qualification,0,sizeof(*qualification));
 for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
  digit_arithmetic_status_t status=evaluate(cases[i].input,&r);
  if(status==cases[i].status&&(status!=DIGIT_ARITHMETIC_OK||r.value==cases[i].value))++passed;
 }
 qualification->tests_executed=(unsigned int)(sizeof(cases)/sizeof(cases[0]));
 qualification->tests_passed=passed;
 qualification->tests_failed=qualification->tests_executed-passed;
 qualification->negative_test_executed=1;
 qualification->negative_test_passed=
  arithmetic_service(NULL,0,&r,sizeof(r),&i,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 return qualification->tests_failed||!qualification->negative_test_passed?
  STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t arithmetic_start(const stnlabz_module_host_t *host){
 if(!host||!host->register_service)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 if(!host->register_service(DIGIT_ARITHMETIC_SERVICE,arithmetic_service,NULL))
  return STNLABZ_MODULE_ERR_START_FAILED;
 arithmetic_host=host;
 if(host->send_message)(void)host->send_message("[ARITHMETIC] arithmetic.evaluate registered");
 return STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t arithmetic_stop(void){
 if(arithmetic_host&&arithmetic_host->unregister_service &&
    !arithmetic_host->unregister_service(DIGIT_ARITHMETIC_SERVICE,NULL))
  return STNLABZ_MODULE_ERR_STOP_FAILED;
 arithmetic_host=NULL;return STNLABZ_MODULE_OK;
}
static const stnlabz_module_descriptor_t descriptor={
 "arithmetic","Digit Arithmetic",1,0,0,
 STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,
 arithmetic_qualify,arithmetic_start,arithmetic_stop
};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &descriptor;}
