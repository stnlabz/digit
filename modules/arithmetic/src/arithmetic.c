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
/* [AI:GPT-6 | 2026-10-09] Fixed-point decimal calculations use a
 * scale of 1000, without floating-point arithmetic. Inputs support up to
 * three fractional digits and integer magnitude <= one million. */
#define DECIMAL_SCALE INT64_C(1000)
static int decimal_operand(const char **cursor,int64_t *out){
 const char *p=*cursor;int sign=1;int64_t whole=0,frac=0;int digits=0;
 skip_spaces(&p);
 if(*p=='+'||*p=='-'){if(*p=='-')sign=-1;++p;}
 if(!isdigit((unsigned char)*p)&&*p!='.')return 0;
 while(isdigit((unsigned char)*p)){
  int digit=*p++-'0';
  if(whole>100000 || (whole==100000&&digit>0))return 0;
  whole=whole*10+digit;++digits;
 }
 if(*p=='.'){
  int fractional=0;
  ++p;
  while(isdigit((unsigned char)*p)){
   if(fractional>=3)return 0;
   frac=frac*10+(*p++-'0');++fractional;
  }
  if(fractional==0)return 0;
  while(fractional++<3)frac*=10;
  digits+=fractional;
 }
 if(digits==0)return 0;
 *out=(whole*DECIMAL_SCALE+frac)*sign;*cursor=p;return 1;
}
static void fixed_text(int64_t value,char *out,size_t cap){
 uint64_t magnitude=(uint64_t)(value<0?-value:value);
 size_t n;
 snprintf(out,cap,"%s%llu.%03llu",value<0?"-":"",
  (unsigned long long)(magnitude/1000),
  (unsigned long long)(magnitude%1000));
 n=strlen(out);
 while(n>0&&out[n-1]=='0')out[--n]=0;
 if(n>0&&out[n-1]=='.')out[--n]=0;
}
static digit_arithmetic_status_t evaluate_decimal(const char *expression,
 digit_arithmetic_result_t *result){
 const char *p=expression;int64_t a,b,v;char op;
 char left[40],right[40],value[40];
 skip_spaces(&p);
 if(strncasecmp(p,"digit",5)==0 &&
    (isspace((unsigned char)p[5])||p[5]==','||p[5]==':')){
  p+=5;if(*p==','||*p==':')++p;skip_spaces(&p);
 }
 if(strncasecmp(p,"what is ",8)==0)p+=8;
 else if(strncasecmp(p,"calculate ",10)==0)p+=10;
 else if(strncasecmp(p,"compute ",8)==0)p+=8;
 if(!decimal_operand(&p,&a))return DIGIT_ARITHMETIC_OUT_OF_RANGE;
 if(!operator_at(&p,&op))return DIGIT_ARITHMETIC_INVALID;
 if(!decimal_operand(&p,&b))return DIGIT_ARITHMETIC_OUT_OF_RANGE;
 skip_spaces(&p);if(*p=='?')++p;skip_spaces(&p);
 if(*p)return DIGIT_ARITHMETIC_INVALID;
 if(op=='/'&&b==0)return DIGIT_ARITHMETIC_DIVIDE_BY_ZERO;
 switch(op){
 case '+':v=a+b;break;
 case '-':v=a-b;break;
 case '*':v=(a*b)/DECIMAL_SCALE;break;
 case '/':v=(a*DECIMAL_SCALE)/b;break;
 default:return DIGIT_ARITHMETIC_INVALID;
 }
 fixed_text(a,left,sizeof(left));
 fixed_text(b,right,sizeof(right));
 fixed_text(v,value,sizeof(value));
 snprintf(result->decimal_answer,sizeof(result->decimal_answer),
  "%s %c %s = %s.",left,op,right,value);
 result->operation=op;
 return DIGIT_ARITHMETIC_OK;
}
/* [AI:GPT-6 | 2026-10-09] Recursive descent for bounded linear algebra.
 * Values are fixed-point (three decimals); x is the only variable.
 * Nonlinear products and variable denominators are rejected. */
typedef struct { int64_t x,c; } linear_value_t;
typedef struct {const char *p;int depth;digit_arithmetic_status_t error;} expression_parser_t;
#define ALG_LIMIT INT64_C(1000000000)
static int alg_limit(int64_t v){return v>=-ALG_LIMIT&&v<=ALG_LIMIT;}
static linear_value_t alg_zero(void){linear_value_t v={0,0};return v;}
static linear_value_t alg_expression(expression_parser_t *parser);
static linear_value_t alg_primary(expression_parser_t *parser){
 linear_value_t v=alg_zero();const char *before;
 if(parser->error)return v;
 skip_spaces(&parser->p);
 if(*parser->p=='+'||*parser->p=='-'){
  char sign=*parser->p++;
  v=alg_primary(parser);
  if(sign=='-'){v.x=-v.x;v.c=-v.c;}
  return v;
 }
 if(*parser->p=='('){
  ++parser->p;
  if(++parser->depth>16){parser->error=DIGIT_ARITHMETIC_INVALID;return v;}
  v=alg_expression(parser);
  skip_spaces(&parser->p);
  if(*parser->p!=')')parser->error=DIGIT_ARITHMETIC_INVALID;
  else ++parser->p;
  --parser->depth;
  return v;
 }
 if((*parser->p=='x'||*parser->p=='X') &&
    !isalnum((unsigned char)parser->p[1])&&parser->p[1]!='_'){
  ++parser->p;v.x=DECIMAL_SCALE;return v;
 }
 before=parser->p;
 if(!decimal_operand(&parser->p,&v.c)){
  parser->p=before;parser->error=DIGIT_ARITHMETIC_INVALID;
 }
 return v;
}
static linear_value_t alg_product(expression_parser_t *parser){
 linear_value_t left=alg_primary(parser);
 while(!parser->error){
  linear_value_t right;char op;int64_t x,c;
  skip_spaces(&parser->p);
  if(*parser->p!='*'&&*parser->p!='/')break;
  op=*parser->p++;
  right=alg_primary(parser);
  if(parser->error)break;
  if(op=='*'){
   if(left.x!=0&&right.x!=0){parser->error=DIGIT_ARITHMETIC_INVALID;break;}
   x=(left.x*right.c+left.c*right.x)/DECIMAL_SCALE;
   c=(left.c*right.c)/DECIMAL_SCALE;
  }else{
   if(right.x!=0){parser->error=DIGIT_ARITHMETIC_INVALID;break;}
   if(right.c==0){parser->error=DIGIT_ARITHMETIC_DIVIDE_BY_ZERO;break;}
   x=(left.x*DECIMAL_SCALE)/right.c;
   c=(left.c*DECIMAL_SCALE)/right.c;
  }
  if(!alg_limit(x)||!alg_limit(c)){parser->error=DIGIT_ARITHMETIC_OUT_OF_RANGE;break;}
  left.x=x;left.c=c;
 }
 return left;
}
static linear_value_t alg_expression(expression_parser_t *parser){
 linear_value_t left=alg_product(parser);
 while(!parser->error){
  linear_value_t right;char op;
  skip_spaces(&parser->p);
  if(*parser->p!='+'&&*parser->p!='-')break;
  op=*parser->p++;
  right=alg_product(parser);
  if(parser->error)break;
  left.x+=(op=='+'?right.x:-right.x);
  left.c+=(op=='+'?right.c:-right.c);
  if(!alg_limit(left.x)||!alg_limit(left.c))parser->error=DIGIT_ARITHMETIC_OUT_OF_RANGE;
 }
 return left;
}
static digit_arithmetic_status_t evaluate_algebra(const char *expression,digit_arithmetic_result_t *result){
 expression_parser_t parser={0};
 linear_value_t lhs,rhs;int64_t coefficient,constant,solution;
 char text[48];
 parser.p=expression;
 skip_spaces(&parser.p);
 if(strncasecmp(parser.p,"digit",5)==0 &&
   (isspace((unsigned char)parser.p[5])||parser.p[5]==','||parser.p[5]==':')){
  parser.p+=5;if(*parser.p==','||*parser.p==':')++parser.p;skip_spaces(&parser.p);
 }
 if(strncasecmp(parser.p,"what is ",8)==0)parser.p+=8;
 else if(strncasecmp(parser.p,"calculate ",10)==0)parser.p+=10;
 else if(strncasecmp(parser.p,"compute ",8)==0)parser.p+=8;
 else if(strncasecmp(parser.p,"solve ",6)==0)parser.p+=6;
 lhs=alg_expression(&parser);
 if(parser.error)return parser.error;
 skip_spaces(&parser.p);
 if(*parser.p=='='){
  ++parser.p;rhs=alg_expression(&parser);
  if(parser.error)return parser.error;
  coefficient=lhs.x-rhs.x;constant=rhs.c-lhs.c;
  if(!alg_limit(coefficient)||!alg_limit(constant))return DIGIT_ARITHMETIC_OUT_OF_RANGE;
  if(coefficient==0){
   snprintf(result->decimal_answer,sizeof(result->decimal_answer),
    "%s",constant==0?"Infinitely many solutions.":"No solution.");
  }else{
   solution=(constant*DECIMAL_SCALE)/coefficient;
   if(!alg_limit(solution))return DIGIT_ARITHMETIC_OUT_OF_RANGE;
   fixed_text(solution,text,sizeof(text));
   snprintf(result->decimal_answer,sizeof(result->decimal_answer),"x = %s.",text);
  }
 }else{
  if(lhs.x!=0)return DIGIT_ARITHMETIC_INVALID;
  fixed_text(lhs.c,text,sizeof(text));
  snprintf(result->decimal_answer,sizeof(result->decimal_answer),"Result = %s.",text);
 }
 skip_spaces(&parser.p);
 if(*parser.p=='?')++parser.p;
 skip_spaces(&parser.p);
 if(*parser.p)return DIGIT_ARITHMETIC_INVALID;
 return DIGIT_ARITHMETIC_OK;
}
/* [AI:GPT-6 | 2026-10-09] Degree-two fixed-point polynomial engine.
 * Polynomial coefficients use scale 1000; no heap, floats, or evaluator code execution. */
typedef struct {int64_t c[3];} polynomial_t;
typedef struct {const char *p;unsigned depth;digit_arithmetic_status_t status;} poly_parser_t;
#define POLY_COEFF_LIMIT INT64_C(10000000)
static polynomial_t poly_zero(void){polynomial_t p={{0,0,0}};return p;}
static int poly_bounds(polynomial_t p){
 unsigned i;for(i=0;i<3;i++)if(p.c[i]>POLY_COEFF_LIMIT||p.c[i]<-POLY_COEFF_LIMIT)return 0;
 return 1;
}
static polynomial_t poly_expression(poly_parser_t *parser);
static polynomial_t poly_primary(poly_parser_t *parser){
 polynomial_t v=poly_zero();const char *start;
 skip_spaces(&parser->p);
 if(*parser->p=='+'||*parser->p=='-'){
  char sign=*parser->p++;
  v=poly_primary(parser);
  if(sign=='-'){unsigned i;for(i=0;i<3;i++)v.c[i]=-v.c[i];}
  return v;
 }
 if(*parser->p=='('){
  ++parser->p;
  if(++parser->depth>16){parser->status=DIGIT_ARITHMETIC_INVALID;return v;}
  v=poly_expression(parser);skip_spaces(&parser->p);
  if(*parser->p!=')')parser->status=DIGIT_ARITHMETIC_INVALID;
  else ++parser->p;
  --parser->depth;
  return v;
 }
 if((*parser->p=='x'||*parser->p=='X')&&!isalnum((unsigned char)parser->p[1])){
  ++parser->p;v.c[1]=DECIMAL_SCALE;return v;
 }
 start=parser->p;
 if(!decimal_operand(&parser->p,&v.c[0])){
  parser->p=start;parser->status=DIGIT_ARITHMETIC_INVALID;
 }
 return v;
}
static polynomial_t poly_power(poly_parser_t *parser){
 polynomial_t v=poly_primary(parser);
 skip_spaces(&parser->p);
 if(*parser->p=='^'){
  unsigned degree=0;polynomial_t square=poly_zero();
  ++parser->p;skip_spaces(&parser->p);
  if(*parser->p!='2'){parser->status=DIGIT_ARITHMETIC_INVALID;return v;}
  ++parser->p;degree=2;
  (void)degree;
  if(v.c[2]){
   parser->status=DIGIT_ARITHMETIC_INVALID;return v;
  }
  if(v.c[1]*v.c[1]>POLY_COEFF_LIMIT*DECIMAL_SCALE){
   parser->status=DIGIT_ARITHMETIC_OUT_OF_RANGE;return v;
  }
  square.c[2]=v.c[1]*v.c[1]/DECIMAL_SCALE;
  square.c[1]=2*v.c[1]*v.c[0]/DECIMAL_SCALE;
  square.c[0]=v.c[0]*v.c[0]/DECIMAL_SCALE;
  v=square;
 }
 return v;
}
static polynomial_t poly_term(poly_parser_t *parser){
 polynomial_t left=poly_power(parser);
 while(!parser->status){
  char op;polynomial_t right,next=poly_zero();unsigned i,j;
  skip_spaces(&parser->p);
  if(*parser->p!='*'&&*parser->p!='/')break;
  op=*parser->p++;right=poly_power(parser);
  if(parser->status)break;
  if(op=='/'){
   if(right.c[1]||right.c[2]){parser->status=DIGIT_ARITHMETIC_INVALID;break;}
   if(!right.c[0]){parser->status=DIGIT_ARITHMETIC_DIVIDE_BY_ZERO;break;}
   for(i=0;i<3;i++)next.c[i]=left.c[i]*DECIMAL_SCALE/right.c[0];
  }else{
   for(i=0;i<3;i++)for(j=0;j<3;j++)if(left.c[i]&&right.c[j]){
    if(i+j>2){parser->status=DIGIT_ARITHMETIC_INVALID;break;}
    next.c[i+j]+=left.c[i]*right.c[j]/DECIMAL_SCALE;
   }
  }
  if(parser->status)break;
  if(!poly_bounds(next)){parser->status=DIGIT_ARITHMETIC_OUT_OF_RANGE;break;}
  left=next;
 }
 return left;
}
static polynomial_t poly_expression(poly_parser_t *parser){
 polynomial_t lhs=poly_term(parser);
 while(!parser->status){
  char op;polynomial_t rhs;unsigned i;
  skip_spaces(&parser->p);
  if(*parser->p!='+'&&*parser->p!='-')break;
  op=*parser->p++;rhs=poly_term(parser);
  if(parser->status)break;
  for(i=0;i<3;i++)lhs.c[i]+=(op=='+'?rhs.c[i]:-rhs.c[i]);
  if(!poly_bounds(lhs))parser->status=DIGIT_ARITHMETIC_OUT_OF_RANGE;
 }
 return lhs;
}
static uint64_t poly_isqrt(uint64_t n){
 uint64_t lo=0,hi=UINT64_C(1000000000),mid;
 while(lo<hi){
  mid=lo+(hi-lo+1)/2;
  if(mid<=n/mid)lo=mid;else hi=mid-1;
 }
 return lo;
}
static digit_arithmetic_status_t evaluate_polynomial(const char *expression,digit_arithmetic_result_t *result){
 poly_parser_t parser={0};polynomial_t lhs,rhs;unsigned i;
 int64_t a,b,c,delta,root1,root2;uint64_t square_root;
 char first[48],second[48];
 parser.p=expression;skip_spaces(&parser.p);
 if(strncasecmp(parser.p,"digit",5)==0 &&
   (isspace((unsigned char)parser.p[5])||parser.p[5]==','||parser.p[5]==':')){
  parser.p+=5;if(*parser.p==','||*parser.p==':')++parser.p;skip_spaces(&parser.p);
 }
 if(strncasecmp(parser.p,"what is ",8)==0)parser.p+=8;
 else if(strncasecmp(parser.p,"calculate ",10)==0)parser.p+=10;
 else if(strncasecmp(parser.p,"compute ",8)==0)parser.p+=8;
 else if(strncasecmp(parser.p,"solve ",6)==0)parser.p+=6;
 lhs=poly_expression(&parser);
 if(parser.status)return parser.status;
 skip_spaces(&parser.p);
 if(*parser.p!='=')return DIGIT_ARITHMETIC_INVALID;
 ++parser.p;rhs=poly_expression(&parser);
 if(parser.status)return parser.status;
 for(i=0;i<3;i++)lhs.c[i]-=rhs.c[i];
 if(!poly_bounds(lhs))return DIGIT_ARITHMETIC_OUT_OF_RANGE;
 skip_spaces(&parser.p);if(*parser.p=='?')++parser.p;skip_spaces(&parser.p);
 if(*parser.p)return DIGIT_ARITHMETIC_INVALID;
 a=lhs.c[2];b=lhs.c[1];c=lhs.c[0];
 if(a==0){
  if(b==0)snprintf(result->decimal_answer,sizeof(result->decimal_answer),"%s",
   c==0?"Infinitely many solutions.":"No solution.");
  else{
   root1=-c*DECIMAL_SCALE/b;
   fixed_text(root1,first,sizeof(first));
   snprintf(result->decimal_answer,sizeof(result->decimal_answer),"x = %s.",first);
  }
  return DIGIT_ARITHMETIC_OK;
 }
 /* Coefficients are bounded to prevent overflow of b*b - 4*a*c. */
 delta=b*b-4*a*c;
 if(delta<0){
  snprintf(result->decimal_answer,sizeof(result->decimal_answer),"No real solutions.");
  return DIGIT_ARITHMETIC_OK;
 }
 square_root=poly_isqrt((uint64_t)delta);
 root1=(-b-(int64_t)square_root)*DECIMAL_SCALE/(2*a);
 root2=(-b+(int64_t)square_root)*DECIMAL_SCALE/(2*a);
 if(root1>root2){int64_t temp=root1;root1=root2;root2=temp;}
 fixed_text(root1,first,sizeof(first));fixed_text(root2,second,sizeof(second));
 /* [AI:GPT-6 | 2026-10-09] Discriminant, not formatted
  * root equality, determines whether a root is repeated. Two distinct
  * roots that collapse at 0.001 precision cannot be reported as one. */
 if(delta==0)
  snprintf(result->decimal_answer,sizeof(result->decimal_answer),"x = %s.",first);
 else if(root1==root2)
  return DIGIT_ARITHMETIC_OUT_OF_RANGE;
 else snprintf(result->decimal_answer,sizeof(result->decimal_answer),
  "%sx = %s and x = %s.",
  square_root*square_root==(uint64_t)delta?"":"Approximately: ",first,second);
 return DIGIT_ARITHMETIC_OK;
}
static digit_arithmetic_status_t evaluate(const char *expression,digit_arithmetic_result_t *result){
 const char *p=expression;int64_t a,b;
 memset(result,0,sizeof(*result));
 if(!expression||!*expression)return DIGIT_ARITHMETIC_INVALID;
 if(strchr(expression,'^')||strstr(expression,"x*x")||strstr(expression,"X*X")||
    strstr(expression,"x * x")||strstr(expression,"X * X")||
    (strchr(expression,'=')&&strchr(expression,'(')&&
     (strchr(expression,'x')||strchr(expression,'X'))))
  return evaluate_polynomial(expression,result);
 /* [AI:GPT-6 | 2026-10-09] Unary signs are not binary operators.
  * Keep signed two-operand integer expressions on the original exact path. */
 {
  const char *scan=expression;unsigned operators=0;char preceding=0;
  while(*scan){
   char c=*scan++;
   if(isspace((unsigned char)c))continue;
   if(c=='*'||c=='/'||
      ((c=='+'||c=='-')&&preceding&&preceding!='+'&&preceding!='-'&&preceding!='*'&&preceding!='/'&&preceding!='('))
      ++operators;
   preceding=c;
  }
  if(operators>1)return evaluate_algebra(expression,result);
 }
 if(strchr(expression,'=')||strchr(expression,'(')||strchr(expression,')'))
  return evaluate_algebra(expression,result);
 if(strchr(expression,'.')!=NULL)return evaluate_decimal(expression,result);
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
 /* [AI:GPT-6 | 2026-10-09] Qualify quadratic behavior during Core hotload. */
 {
  static const struct {const char *equation,*answer;} quadratics[]={
   {"solve x^2 - 5*x + 6 = 0","x = 2 and x = 3."},
   {"solve x^2 - 4*x + 4 = 0","x = 2."},
   {"solve x^2 + 1 = 0","No real solutions."},
   {"solve (x+1)*(x+2) = 0","x = -2 and x = -1."}
  };
  for(i=0;i<sizeof(quadratics)/sizeof(quadratics[0]);++i){
   digit_arithmetic_status_t status=evaluate(quadratics[i].equation,&r);
   if(status==DIGIT_ARITHMETIC_OK &&
      strcmp(r.decimal_answer,quadratics[i].answer)==0)++passed;
  }
 }
 qualification->tests_executed=(unsigned int)(sizeof(cases)/sizeof(cases[0])+4);
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
 "arithmetic","Digit Arithmetic",1,3,1,
 STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,
 arithmetic_qualify,arithmetic_start,arithmetic_stop
};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &descriptor;}
