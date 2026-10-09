#include <ctype.h>
#include <stdint.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>

#include "response.h"
#include "reasoning.h"

/* [AI:GPT-5.6 Sol | 2026-10-06T21:32:00Z] Removed operational LLM generation; Response now renders selected authorized evidence deterministically and submits output to Validator. */
/* [AI:GPT-5.6 Sol | 2026-10-06T22:33:00Z] Removed subject-specific PHP query expansion. Response retrieval is subject-agnostic; knowledge comes from authorized sources, not hard-coded domain facts. */
/* [AI:GPT-5.6 Sol | 2026-10-06T23:58:00Z] Response now consumes Dispatcher structured Intent envelopes. Retrieval uses the original request/subject rather than Intent metadata, and EXPLAIN assembles multiple grounded evidence records instead of collapsing to one lexical hit. */
/* [AI:GPT-5.6 Sol | 2026-10-07T00:31:00Z] EXPLAIN evidence organization is delegated to reasoning.explain. Response supplies only selected authorized evidence and renders the returned grounded explanation before Validator output validation. */
/* [AI:GPT-5.6 Sol | 2026-10-07T00:00:00Z] Prevented zero-score evidence from reaching answers and constrained structured CONVERSATION responses to explicit operator-learned records. */
/* [AI:GPT-5.6 Sol | 2026-10-07T00:10:00Z] Structured CONVERSATION now applies explicit operator-learned can-be-responded-to-with relationships instead of reciting the relationship record. */
/* [AI:GPT-5.6 Sol | 2026-10-07T01:38:00Z] Structured COMPARE delegates evidence-backed comparison construction to reasoning.compare rather than returning the highest lexical retrieval hit. */
/* [AI:GPT-5.6 Sol | 2026-10-07T01:51:00Z] COMPARE retrieval uses the established Intent subject expression rather than the operation-bearing request, preventing the word "compare" from dominating evidence ranking. */
/* [AI:GPT-5.6 Sol | 2026-10-07T01:58:00Z] COMPARE retrieves and ranks each established subject independently before reasoning, preventing combined-query records from displacing subject-specific evidence. */
/* [AI:GPT-5.6 Sol | 2026-10-07T02:02:00Z] Added forward declarations for retrieval helpers now invoked by structured COMPARE before their definitions; behavior unchanged. */
/* [AI:GPT-5.6 Sol | 2026-10-07T02:10:00Z] Outbound COMPARE validation now receives the evidence actually used to construct the comparison rather than the obsolete pre-compare combined-query selection. */

#define CORPUS_SEARCH_SERVICE "corpus.search"
#define SOURCE_PROJECTS_SERVICE "source.projects"
#define SOURCE_SEARCH_SERVICE "source.search"
#define VALIDATOR_INBOUND_SERVICE "validator.inbound"
#define VALIDATOR_OUTBOUND_SERVICE "validator.outbound"
#define VALIDATOR_TEXT_MAX 4096
#define VALIDATOR_REASON_MAX 256
#define CORPUS_MAX 16
#define CORPUS_TEXT_MAX 4096
#define SOURCE_PROJECT_MAX 64
#define SOURCE_PATH_MAX 512
#define SOURCE_QUERY_MAX 256
#define SOURCE_PROJECTS_MAX 64
#define SOURCE_SEARCH_MAX 16
#define TERM_MAX 64
#define TERM_COUNT 32
#define SELECTED_MAX 6

typedef struct { char id[65]; char category[64]; char source[256]; char text[CORPUS_TEXT_MAX]; } corpus_record_t;
typedef struct { char query[CORPUS_TEXT_MAX]; } corpus_search_request_t;
typedef struct { size_t count; corpus_record_t records[CORPUS_MAX]; } corpus_result_t;
typedef struct { char name[SOURCE_PROJECT_MAX]; } source_project_t;
typedef struct { size_t count; source_project_t projects[SOURCE_PROJECTS_MAX]; } source_projects_result_t;
typedef struct { char project[SOURCE_PROJECT_MAX]; char query[SOURCE_QUERY_MAX]; } source_search_request_t;
typedef struct { char project[SOURCE_PROJECT_MAX]; char path[SOURCE_PATH_MAX]; size_t line; char text[CORPUS_TEXT_MAX]; } source_match_t;
typedef struct { size_t count; source_match_t matches[SOURCE_SEARCH_MAX]; } source_search_result_t;
typedef struct { corpus_record_t record; unsigned int score; unsigned int matches; } ranked_record_t;
static void grounded_fallback(const char *question, ranked_record_t selected[SELECTED_MAX], size_t selected_count, digit_response_result_t *output);
static int complete_phrase_match(const char *phrase, const char *text);
static int collect_corpus(const char *question, corpus_result_t *evidence);
static size_t select_evidence(const char *question, const corpus_result_t *evidence, ranked_record_t selected[SELECTED_MAX]);
static void trace_evidence(const char *question, const corpus_result_t *evidence, ranked_record_t selected[SELECTED_MAX], size_t selected_count);

typedef enum { VALIDATOR_PASS=0, VALIDATOR_FAIL=1 } validator_status_t;
typedef enum { VALIDATOR_CONFIDENCE_LOW=0, VALIDATOR_CONFIDENCE_MODERATE=1, VALIDATOR_CONFIDENCE_HIGH=2 } validator_confidence_t;
typedef struct { char raw[VALIDATOR_TEXT_MAX]; } validator_inbound_request_t;
typedef struct { validator_status_t status; validator_confidence_t confidence; char normalized[VALIDATOR_TEXT_MAX]; char reason[VALIDATOR_REASON_MAX]; } validator_inbound_result_t;
typedef struct { char raw[VALIDATOR_TEXT_MAX]; char normalized[VALIDATOR_TEXT_MAX]; char candidate[VALIDATOR_TEXT_MAX]; char evidence[VALIDATOR_TEXT_MAX]; unsigned int attempt; } validator_outbound_request_t;
typedef struct { validator_status_t status; int retry_allowed; char reason[VALIDATOR_REASON_MAX]; } validator_outbound_result_t;

static const stnlabz_module_host_t *response_host=NULL;

static int stopword(const char *word){static const char *words[]={"a","an","and","are","as","at","be","been","but","by","can","could","did","do","does","for","from","had","has","have","how","i","if","in","into","is","it","its","may","must","of","on","or","should","that","the","their","then","there","these","they","this","to","was","were","what","when","where","which","who","why","will","with","would","your"};size_t i;for(i=0;i<sizeof(words)/sizeof(words[0]);++i)if(strcmp(word,words[i])==0)return 1;return 0;}
static size_t terms(const char *text,char out[TERM_COUNT][TERM_MAX]){char word[TERM_MAX];size_t count=0,w=0,i;unsigned char ch;if(text==NULL)return 0;for(i=0;;++i){ch=(unsigned char)text[i];if(isalnum(ch)||ch=='_'||ch=='-'){if(w+1<sizeof(word))word[w++]=(char)tolower(ch);}else if(w>0){size_t j;int duplicate=0;word[w]='\0';if(!stopword(word)){for(j=0;j<count;++j)if(strcmp(out[j],word)==0){duplicate=1;break;}if(!duplicate&&count<TERM_COUNT){snprintf(out[count],TERM_MAX,"%s",word);++count;}}w=0;}if(ch=='\0')break;}return count;}
static int has_term(char list[TERM_COUNT][TERM_MAX],size_t count,const char *term){size_t i;for(i=0;i<count;++i)if(strcmp(list[i],term)==0)return 1;return 0;}
static void canonical_text(const char *src,char *dst,size_t size){size_t i,o=0;int space=0;if(dst==NULL||size==0)return;dst[0]='\0';if(src==NULL)return;for(i=0;src[i]!='\0'&&o+1<size;++i){unsigned char ch=(unsigned char)src[i];if(isalnum(ch)){if(space&&o>0&&o+1<size)dst[o++]=' ';dst[o++]=(char)tolower(ch);space=0;}else if(o>0)space=1;}dst[o]='\0';}
static int question_like_record(const char *question,const char *record_text){char q[CORPUS_TEXT_MAX],r[CORPUS_TEXT_MAX];char qt[TERM_COUNT][TERM_MAX],rt[TERM_COUNT][TERM_MAX];size_t qn,rn,i,shared=0;if(question==NULL||record_text==NULL)return 0;canonical_text(question,q,sizeof(q));canonical_text(record_text,r,sizeof(r));if(q[0]=='\0'||r[0]=='\0')return 0;if(strcmp(q,r)==0)return 1;if(strlen(q)>=12&&strstr(r,q)!=NULL&&strlen(r)<=strlen(q)+32)return 1;if(strlen(r)>=12&&strstr(q,r)!=NULL&&strlen(q)<=strlen(r)+32)return 1;memset(qt,0,sizeof(qt));memset(rt,0,sizeof(rt));qn=terms(question,qt);rn=terms(record_text,rt);if(qn==0||rn==0)return 0;for(i=0;i<rn;++i)if(has_term(qt,qn,rt[i]))++shared;return rn<=qn+1&&shared==rn&&shared*4U>=qn*3U;}
static int ordinal_token_value(const char *word){static const char *ordinal[][2]={{"first","1st"},{"second","2nd"},{"third","3rd"},{"fourth","4th"},{"fifth","5th"},{"sixth","6th"},{"seventh","7th"},{"eighth","8th"},{"ninth","9th"},{"tenth","10th"}};size_t i,j;for(i=0;i<sizeof(ordinal)/sizeof(ordinal[0]);++i)for(j=0;j<2;++j)if(strcmp(word,ordinal[i][j])==0)return (int)i+1;return 0;}
static int cardinal_token_value(const char *word){static const char *cardinal[][2]={{"one","1"},{"two","2"},{"three","3"},{"four","4"},{"five","5"},{"six","6"},{"seven","7"},{"eight","8"},{"nine","9"},{"ten","10"}};size_t i,j;for(i=0;i<sizeof(cardinal)/sizeof(cardinal[0]);++i)for(j=0;j<2;++j)if(strcmp(word,cardinal[i][j])==0)return (int)i+1;return 0;}
static int reference_value(const char *text){char list[TERM_COUNT][TERM_MAX];size_t n,i;int value;memset(list,0,sizeof(list));n=terms(text,list);for(i=0;i<n;++i)if((value=ordinal_token_value(list[i]))>0)return value;for(i=0;i<n;++i){value=cardinal_token_value(list[i]);if(value==0)continue;if(i>0&&(strcmp(list[i-1],"number")==0||strcmp(list[i-1],"no")==0||strcmp(list[i-1],"order")==0||strcmp(list[i-1],"item")==0||strcmp(list[i-1],"rule")==0||strcmp(list[i-1],"step")==0))return value;if(i+1<n&&(strcmp(list[i+1],"number")==0||strcmp(list[i+1],"order")==0||strcmp(list[i+1],"item")==0||strcmp(list[i+1],"rule")==0||strcmp(list[i+1],"step")==0))return value;}return 0;}
/* [AI:GPT-6 | 2026-10-09] Do not substitute unrelated retrieved arithmetic
 * text for a calculation. No arithmetic executor is registered here. */
static int arithmetic_question(const char *text){
 char token[64];size_t i=0,j=0;int operands=0,operation=0;
 if(!text)return 0;
 for(;;){
  unsigned char c=(unsigned char)text[i++];
  if(isalnum(c)){if(j+1<sizeof(token))token[j++]=(char)tolower(c);}
  else {
   if(j){
    token[j]=0;
    if(!strcmp(token,"plus")||!strcmp(token,"minus")||
       !strcmp(token,"times")||!strcmp(token,"multiply")||
       !strcmp(token,"divided")||!strcmp(token,"divide")||!strcmp(token,"over")||!strcmp(token,"add")||
       !strcmp(token,"subtract")||!strcmp(token,"x"))operation=1;
    if(isdigit((unsigned char)token[0])||!strcmp(token,"one")||
       !strcmp(token,"two")||!strcmp(token,"three")||
       !strcmp(token,"four")||!strcmp(token,"five")||
       !strcmp(token,"six")||!strcmp(token,"seven")||
       !strcmp(token,"eight")||!strcmp(token,"nine")||
       !strcmp(token,"ten"))operands++;
    j=0;
   }
   if(c=='+'||c=='-'||c=='*'||c=='/'||(c==0xC3 && (unsigned char)text[i]==0xB7))operation=1;
   if(!c)break;
  }
 }
 return operation&&operands>=2;
}
/* [AI:GPT-6 | 2026-10-09] Bounded two-operand arithmetic is computed,
 * never retrieved as lexical evidence. Limit operands to one billion so
 * addition, subtraction, and multiplication remain within signed int64. */
static int arithmetic_operand(const char **cursor,int64_t *value){
 static const char *const names[]={"zero","one","two","three","four","five","six","seven","eight","nine","ten"};
 const char *p=*cursor;int sign=1;int64_t number=0;size_t i;
 while(isspace((unsigned char)*p))++p;
 if(*p=='-'){sign=-1;++p;}else if(*p=='+')++p;
 if(isdigit((unsigned char)*p)){
  do{
   int digit=*p-'0';
   if(number>1000000000/10 || (number==1000000000/10 && digit>1000000000%10))return 0;
   number=number*10+digit;++p;
  }while(isdigit((unsigned char)*p));
 }else{
  for(i=0;i<sizeof(names)/sizeof(names[0]);++i){
   size_t n=strlen(names[i]);
   if(strncasecmp(p,names[i],n)==0&&!isalnum((unsigned char)p[n])&&p[n]!='_'){
    number=(int64_t)i;p+=n;break;
   }
  }
  if(i==sizeof(names)/sizeof(names[0]))return 0;
 }
 *value=number*sign;*cursor=p;return 1;
}
static int arithmetic_calculate(const char *text,char *answer,size_t cap){
 const char *p=text;int64_t a,b,result;char operation=0;
 if(!p||!answer||cap==0)return 0;
 while(isspace((unsigned char)*p))++p;
 /* [AI:GPT-6 | 2026-10-09] Live requests address Digit by name.
  * Strip only the explicit wake word, never arbitrary leading text. */
 if(strncasecmp(p,"digit",5)==0 &&
    (isspace((unsigned char)p[5])||p[5]==','||p[5]==':')){
  p+=5;
  if(*p==','||*p==':')++p;
  while(isspace((unsigned char)*p))++p;
 }
 if(strncasecmp(p,"what is ",8)==0)p+=8;
 else if(strncasecmp(p,"calculate ",10)==0)p+=10;
 else if(strncasecmp(p,"compute ",8)==0)p+=8;
 if(!arithmetic_operand(&p,&a))return 0;
 while(isspace((unsigned char)*p))++p;
 if(*p=='+'||*p=='-'||*p=='*'||*p=='/'){operation=*p++;}
 else if((unsigned char)p[0]==0xC3 && (unsigned char)p[1]==0xB7){operation='/';p+=2;}
 else{
  static const struct {const char *word;char operation;} ops[]={
   {"plus", '+'},{"minus",'-'},{"times",'*'},{"x",'*'},{"divided by",'/'},{"divide by",'/'},{"over",'/'}
  };
  size_t i;
  for(i=0;i<sizeof(ops)/sizeof(ops[0]);++i){
   size_t n=strlen(ops[i].word);
   if(strncasecmp(p,ops[i].word,n)==0&&!isalpha((unsigned char)p[n])){
    operation=ops[i].operation;p+=n;break;
   }
  }
 }
 if(!operation||!arithmetic_operand(&p,&b))return 0;
 while(isspace((unsigned char)*p))++p;
 if(*p=='?')++p;
 while(isspace((unsigned char)*p))++p;
 if(*p!='\0')return 0;
 if(operation=='/'&&b==0){
  snprintf(answer,cap,"Division by zero is undefined.");return 1;
 }
 switch(operation){
 case '+':result=a+b;break;
 case '-':result=a-b;break;
 case '*':result=a*b;break;
 case '/':result=a/b;break;
 default:return 0;
 }
 if(operation=='/'&&a%b!=0)
  snprintf(answer,cap,"%lld / %lld = %lld remainder %lld.",
   (long long)a,(long long)b,(long long)result,(long long)(a%b));
 else
  snprintf(answer,cap,"%lld %c %lld = %lld.",
   (long long)a,operation,(long long)b,(long long)result);
 return 1;
}
static int conversational_greeting(const char *input,char *answer,size_t answer_size){char normalized[128];size_t i=0,o=0;const char *reply=NULL;if(input==NULL||answer==NULL||answer_size==0)return 0;while(input[i]!='\0'&&isspace((unsigned char)input[i]))++i;while(input[i]!='\0'&&o+1<sizeof(normalized)){unsigned char ch=(unsigned char)input[i++];if(isalnum(ch))normalized[o++]=(char)tolower(ch);else if(isspace(ch)&&o>0&&normalized[o-1]!=' ')normalized[o++]=' ';}while(o>0&&normalized[o-1]==' ')--o;normalized[o]='\0';if(strcmp(normalized,"good morning")==0)reply="Good morning.";else if(strcmp(normalized,"good afternoon")==0)reply="Good afternoon.";else if(strcmp(normalized,"good evening")==0)reply="Good evening.";else if(strcmp(normalized,"hello")==0||strcmp(normalized,"hi")==0||strcmp(normalized,"hey")==0||strcmp(normalized,"hello digit")==0||strcmp(normalized,"hi digit")==0||strcmp(normalized,"hey digit")==0)reply="Hello.";if(reply==NULL)return 0;snprintf(answer,answer_size,"%s",reply);return 1;}
typedef struct { char intent[32]; char target[32]; char subject[256]; char request[DIGIT_RESPONSE_QUESTION_MAX]; int structured; } response_intent_t;
static void trim_line(char *text){size_t n;if(text==NULL)return;n=strlen(text);while(n>0&&(text[n-1]=='\r'||text[n-1]=='\n'||isspace((unsigned char)text[n-1])))text[--n]='\0';}
static void parse_intent_envelope(const char *input,response_intent_t *parsed){const char *p,*line;size_t n;if(parsed==NULL)return;memset(parsed,0,sizeof(*parsed));if(input==NULL)return;if(strncmp(input,"INTENT: ",8)!=0){snprintf(parsed->request,sizeof(parsed->request),"%s",input);return;}p=input+8;line=strchr(p,'\n');if(line==NULL){snprintf(parsed->request,sizeof(parsed->request),"%s",input);return;}n=(size_t)(line-p);if(n>=sizeof(parsed->intent))n=sizeof(parsed->intent)-1;memcpy(parsed->intent,p,n);parsed->intent[n]='\0';p=line+1;if(strncmp(p,"TARGET: ",8)!=0){snprintf(parsed->request,sizeof(parsed->request),"%s",input);return;}p+=8;line=strchr(p,'\n');if(line==NULL){snprintf(parsed->request,sizeof(parsed->request),"%s",input);return;}n=(size_t)(line-p);if(n>=sizeof(parsed->target))n=sizeof(parsed->target)-1;memcpy(parsed->target,p,n);parsed->target[n]='\0';p=line+1;if(strncmp(p,"SUBJECT: ",9)!=0){snprintf(parsed->request,sizeof(parsed->request),"%s",input);return;}p+=9;line=strchr(p,'\n');if(line==NULL){snprintf(parsed->request,sizeof(parsed->request),"%s",input);return;}n=(size_t)(line-p);if(n>=sizeof(parsed->subject))n=sizeof(parsed->subject)-1;memcpy(parsed->subject,p,n);parsed->subject[n]='\0';trim_line(parsed->subject);p=line+1;if(strncmp(p,"REQUEST: ",9)!=0){snprintf(parsed->request,sizeof(parsed->request),"%s",input);return;}snprintf(parsed->request,sizeof(parsed->request),"%s",p+9);parsed->structured=1;}
static const char *retrieval_text(const response_intent_t *intent){if(intent==NULL)return "";if(strcmp(intent->intent,"CONVERSATION")==0)return intent->request;if(intent->subject[0]!='\0'&&(strcmp(intent->intent,"DEFINE")==0||strcmp(intent->intent,"EXPLAIN")==0||strcmp(intent->intent,"COMPARE")==0))return intent->subject;return intent->request;}
/* [AI:GPT-6 | 2026-10-08] Render retained lesson definitions as grounded prose without entry metadata or examples. */
static int render_lesson_definition(const char *subject, ranked_record_t selected[SELECTED_MAX], size_t count, char *answer, size_t answer_size)
{
    size_t i;
    if(subject==NULL||subject[0]=='\0'||answer==NULL||answer_size==0)return 0;
    for(i=0;i<count;++i){
        const char *text=selected[i].record.text,*definition,*end,*delimiter;
        size_t length;
        int written;
        if(strncmp(selected[i].record.source,"lesson:",7)!=0||strncmp(text,"ENTRY ",6)!=0)continue;
        if(!complete_phrase_match(subject,text))continue;
        delimiter=strstr(text," — ");
        if(delimiter!=NULL)definition=delimiter+strlen(" — ");
        else if((delimiter=strstr(text," – "))!=NULL)definition=delimiter+strlen(" – ");
        else continue;
        while(*definition==' '||*definition=='\t')++definition;
        end=strstr(definition,". Example:");
        if(end==NULL)end=strchr(definition,'.');
        if(end==NULL)end=definition+strlen(definition);
        while(end>definition&&isspace((unsigned char)end[-1]))--end;
        length=(size_t)(end-definition);
        if(length==0||length>=answer_size||length>4096U)continue;
        written=snprintf(answer,answer_size,"The expression describes %.*s.",(int)length,definition);
        if(written>0&&(size_t)written<answer_size)return 1;
    }
    return 0;
}
static void render_intent_answer(const response_intent_t *intent,const char *question,ranked_record_t selected[SELECTED_MAX],size_t selected_count,digit_response_result_t *output,char *comparison_evidence,size_t comparison_evidence_size){digit_reasoning_explain_request_t request;digit_reasoning_explain_result_t result;stnlabz_module_result_t status;size_t used=0,i,count;if(intent!=NULL&&strcmp(intent->intent,"CONVERSATION")==0){size_t i;output->answered=1;for(i=0;i<selected_count;++i)if(strcmp(selected[i].record.category,"OPERATOR_LEARNED")==0&&strcmp(selected[i].record.source,"interface:learn")==0){const char *relation=strstr(selected[i].record.text,"can be responded to with");if(relation!=NULL){const char *p=relation+strlen("can be responded to with");char choices[8][128];size_t choice_count=0;while(*p!='\0'&&choice_count<8){const char *start;size_t n;while(*p!='\0'&&*p!='"')++p;if(*p=='\0')break;start=++p;while(*p!='\0'&&*p!='"')++p;if(*p!='"')break;n=(size_t)(p-start);if(n>0&&n<sizeof(choices[0])){memcpy(choices[choice_count],start,n);choices[choice_count][n]='\0';++choice_count;}if(*p=='"')++p;}if(choice_count>0){unsigned long hash=5381UL;const unsigned char *q=(const unsigned char *)question;while(*q!='\0')hash=((hash<<5)+hash)+(unsigned long)*q++;/* [AI:GPT-6 | 2026-10-08] Do not select a learned social reply that merely echoes the operator. */{size_t offset;for(offset=0;offset<choice_count;++offset){const char *reply=choices[(hash+offset)%choice_count];char qnorm[CORPUS_TEXT_MAX],rnorm[CORPUS_TEXT_MAX];canonical_text(question,qnorm,sizeof(qnorm));canonical_text(reply,rnorm,sizeof(rnorm));if(strcmp(qnorm,rnorm)==0)continue;snprintf(output->answer,sizeof(output->answer),"%s",reply);return;}}}}}snprintf(output->answer,sizeof(output->answer),"I don't have enough information to answer that.");return;}if(intent!=NULL&&strcmp(intent->intent,"COMPARE")==0){digit_reasoning_compare_request_t cr;digit_reasoning_compare_result_t co;corpus_result_t left_ev,right_ev;ranked_record_t left_sel[SELECTED_MAX],right_sel[SELECTED_MAX];char left[DIGIT_REASONING_SUBJECT_MAX],right[DIGIT_REASONING_SUBJECT_MAX];const char *p=question,*andp=NULL;size_t n,left_count=0,right_count=0,k=0;p=question;while(*p&&isspace((unsigned char)*p))++p;andp=strstr(p," and ");if(andp==NULL){output->answered=1;snprintf(output->answer,sizeof(output->answer),"I don't have two established subjects to compare.");return;}n=(size_t)(andp-p);while(n>0&&isspace((unsigned char)p[n-1]))--n;if(n>=sizeof(left))n=sizeof(left)-1;memcpy(left,p,n);left[n]='\0';p=andp+5;while(*p&&isspace((unsigned char)*p))++p;snprintf(right,sizeof(right),"%s",p);trim_line(right);n=strlen(right);while(n>0&&(right[n-1]=='?'||right[n-1]=='.'||right[n-1]=='!'))right[--n]='\0';memset(&left_ev,0,sizeof(left_ev));memset(&right_ev,0,sizeof(right_ev));memset(left_sel,0,sizeof(left_sel));memset(right_sel,0,sizeof(right_sel));if(!collect_corpus(left,&left_ev)||!collect_corpus(right,&right_ev)){output->answered=1;snprintf(output->answer,sizeof(output->answer),"I can't access retained information right now.");return;}left_count=select_evidence(left,&left_ev,left_sel);right_count=select_evidence(right,&right_ev,right_sel);trace_evidence(left,&left_ev,left_sel,left_count);trace_evidence(right,&right_ev,right_sel,right_count);memset(&cr,0,sizeof(cr));memset(&co,0,sizeof(co));snprintf(cr.left,sizeof(cr.left),"%s",left);snprintf(cr.right,sizeof(cr.right),"%s",right);for(i=0;i<left_count&&k<DIGIT_REASONING_EVIDENCE_MAX;++i)snprintf(cr.evidence[k++],sizeof(cr.evidence[0]),"%s",left_sel[i].record.text);for(i=0;i<right_count&&k<DIGIT_REASONING_EVIDENCE_MAX;++i){size_t d;int duplicate=0;for(d=0;d<k;++d)if(strcmp(cr.evidence[d],right_sel[i].record.text)==0){duplicate=1;break;}if(!duplicate)snprintf(cr.evidence[k++],sizeof(cr.evidence[0]),"%s",right_sel[i].record.text);}cr.evidence_count=k;
 /* [AI:GPT-6 | 2026-10-09] Outbound Validator receives the actual
  * retained comparison records, never the candidate's own words. */
 if(comparison_evidence&&comparison_evidence_size){
  size_t off=0,e;
  comparison_evidence[0]='\0';
  for(e=0;e<k;e++){
   int n=snprintf(comparison_evidence+off,comparison_evidence_size-off,
      "%s%s",e?"\n":"",cr.evidence[e]);
   if(n<0||(size_t)n>=comparison_evidence_size-off){
    comparison_evidence[0]='\0';break;
   }
   off+=(size_t)n;
  }
 }
 used=0;status=response_host->invoke_service(DIGIT_REASONING_COMPARE_SERVICE,&cr,sizeof(cr),&co,sizeof(co),&used);output->answered=1;if(status!=STNLABZ_MODULE_OK||used!=sizeof(co)||!co.compared||co.comparison[0]=='\0'){snprintf(output->answer,sizeof(output->answer),"I don't have enough grounded information to compare those subjects.");return;}snprintf(output->answer,sizeof(output->answer),"%s",co.comparison);return;}if(intent!=NULL&&(strcmp(intent->intent,"EXPLAIN")==0||strcmp(intent->intent,"DEFINE")==0)&&selected_count>0&&render_lesson_definition(question,selected,selected_count,output->answer,sizeof(output->answer))){output->answered=1;return;}if(intent==NULL||strcmp(intent->intent,"EXPLAIN")!=0){grounded_fallback(question,selected,selected_count,output);return;}output->answered=1;if(selected_count==0||response_host==NULL||response_host->invoke_service==NULL){snprintf(output->answer,sizeof(output->answer),"I don't have enough information to answer that.");return;}memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));/* [AI:GPT-6 | 2026-10-08] EXPLAIN must use the same resolved subject as evidence retrieval. */snprintf(request.subject,sizeof(request.subject),"%.*s",(int)(sizeof(request.subject)-1U),question);count=selected_count>DIGIT_REASONING_EVIDENCE_MAX?DIGIT_REASONING_EVIDENCE_MAX:selected_count;for(i=0;i<count;++i)snprintf(request.evidence[i],sizeof(request.evidence[i]),"%s",selected[i].record.text);request.evidence_count=count;status=response_host->invoke_service(DIGIT_REASONING_EXPLAIN_SERVICE,&request,sizeof(request),&result,sizeof(result),&used);if(status!=STNLABZ_MODULE_OK||used!=sizeof(result)||!result.explained||result.explanation[0]=='\0'){snprintf(output->answer,sizeof(output->answer),"I don't have enough grounded information to explain that.");return;}snprintf(output->answer,sizeof(output->answer),"%s",result.explanation);}
static int evidence_requested(const char *question){char qt[TERM_COUNT][TERM_MAX];size_t qn,i;static const char *words[]={"evidence","prove","proof","provenance","citation","citations","source","sources"};memset(qt,0,sizeof(qt));qn=terms(question,qt);for(i=0;i<qn;++i){size_t j;for(j=0;j<sizeof(words)/sizeof(words[0]);++j)if(strcmp(qt[i],words[j])==0)return 1;}return 0;}
static int source_intent(const char *question){char qt[TERM_COUNT][TERM_MAX];size_t qn,i;static const char *engineering[]={"abi","code","source","function","functions","module","modules","implementation","file","files","header","headers","struct","service","services","compile","compiler","build","engineering","evidence"};memset(qt,0,sizeof(qt));qn=terms(question,qt);for(i=0;i<qn;++i){size_t j;for(j=0;j<sizeof(engineering)/sizeof(engineering[0]);++j)if(strcmp(qt[i],engineering[j])==0)return 1;}return 0;}
static int collect_corpus(const char *question,corpus_result_t *evidence){corpus_search_request_t request;stnlabz_module_result_t result;size_t used=0;char query_terms[TERM_COUNT][TERM_MAX];size_t count,i,offset=0;if(question==NULL||evidence==NULL||response_host==NULL||response_host->invoke_service==NULL)return 0;memset(&request,0,sizeof(request));memset(query_terms,0,sizeof(query_terms));count=terms(question,query_terms);if(count==0)return 1;for(i=0;i<count;++i){int written=snprintf(request.query+offset,sizeof(request.query)-offset,"%s%s",i?" ":"",query_terms[i]);if(written<=0||(size_t)written>=sizeof(request.query)-offset)break;offset+=(size_t)written;}memset(evidence,0,sizeof(*evidence));result=response_host->invoke_service(CORPUS_SEARCH_SERVICE,&request,sizeof(request),evidence,sizeof(*evidence),&used);return result==STNLABZ_MODULE_OK&&used==sizeof(*evidence);}
static int duplicate_source(const corpus_result_t *evidence,const char *source,const char *text){size_t i;for(i=0;i<evidence->count;++i)if(strcmp(evidence->records[i].category,"source")==0&&strcmp(evidence->records[i].source,source)==0&&strcmp(evidence->records[i].text,text)==0)return 1;return 0;}
static void collect_source(const char *question,corpus_result_t *evidence){source_projects_result_t projects;char qt[TERM_COUNT][TERM_MAX];size_t qn,p,q,used=0;stnlabz_module_result_t result;if(question==NULL||evidence==NULL||response_host==NULL||response_host->invoke_service==NULL||!source_intent(question)||evidence->count>=CORPUS_MAX)return;memset(&projects,0,sizeof(projects));result=response_host->invoke_service(SOURCE_PROJECTS_SERVICE,NULL,0,&projects,sizeof(projects),&used);if(result!=STNLABZ_MODULE_OK||used!=sizeof(projects))return;memset(qt,0,sizeof(qt));qn=terms(question,qt);for(q=0;q<qn&&evidence->count<CORPUS_MAX;++q){if(strlen(qt[q])<3)continue;for(p=0;p<projects.count&&p<SOURCE_PROJECTS_MAX&&evidence->count<CORPUS_MAX;++p){source_search_request_t request;source_search_result_t found;size_t search_used=0,m;memset(&request,0,sizeof(request));snprintf(request.project,sizeof(request.project),"%s",projects.projects[p].name);snprintf(request.query,sizeof(request.query),"%s",qt[q]);memset(&found,0,sizeof(found));result=response_host->invoke_service(SOURCE_SEARCH_SERVICE,&request,sizeof(request),&found,sizeof(found),&search_used);if(result!=STNLABZ_MODULE_OK||search_used!=sizeof(found))continue;for(m=0;m<found.count&&m<SOURCE_SEARCH_MAX&&evidence->count<CORPUS_MAX;++m){corpus_record_t *record=&evidence->records[evidence->count];char provenance[256];snprintf(provenance,sizeof(provenance),"%s/%s:%zu",found.matches[m].project,found.matches[m].path,found.matches[m].line);if(duplicate_source(evidence,provenance,found.matches[m].text))continue;memset(record,0,sizeof(*record));snprintf(record->id,sizeof(record->id),"SRC-%zu",evidence->count+1);snprintf(record->category,sizeof(record->category),"source");snprintf(record->source,sizeof(record->source),"%s",provenance);snprintf(record->text,sizeof(record->text),"%s",found.matches[m].text);++evidence->count;}}}}
static unsigned int lesson_bonus(const char *question,const corpus_record_t *record){char topic[TERM_MAX];const char *start,*end;size_t len;if(record==NULL||strncmp(record->source,"lesson:",7)!=0)return 0;start=record->source+7;end=strstr(start,".txt");if(end==NULL||end<=start)return 150U;len=(size_t)(end-start);if(len>=sizeof(topic))len=sizeof(topic)-1;memcpy(topic,start,len);topic[len]='\0';if(question!=NULL){char qt[TERM_COUNT][TERM_MAX];size_t qn;memset(qt,0,sizeof(qt));qn=terms(question,qt);if(has_term(qt,qn,topic))return 300U;}return 150U;}
/* [AI:GPT-6 | 2026-10-08] Prefer contiguous multiword query expressions in evidence over isolated token overlap. */
/* Preserve repeated terms and their original order for phrase matching.
 * General retrieval terms() intentionally deduplicates; phrases cannot. */
static size_t phrase_terms(const char *text,char out[TERM_COUNT][TERM_MAX])
{
    char word[TERM_MAX];size_t count=0,w=0,i;unsigned char ch;
    if(text==NULL)return 0;
    for(i=0;;++i){
        ch=(unsigned char)text[i];
        if(isalnum(ch)||ch=='_'||ch=='-'){
            if(w+1<sizeof(word))word[w++]=(char)tolower(ch);
        }else if(w>0){
            word[w]='\0';
            if(!stopword(word)&&count<TERM_COUNT){
                snprintf(out[count],TERM_MAX,"%s",word);
                ++count;
            }
            w=0;
        }
        if(ch=='\0')break;
    }
    return count;
}
static unsigned int phrase_match_bonus(const char *question,const char *record_text)
{
    char qt[TERM_COUNT][TERM_MAX],rt[TERM_COUNT][TERM_MAX];
    size_t qn,rn,i,j,best=0,run;
    memset(qt,0,sizeof(qt));memset(rt,0,sizeof(rt));
    qn=phrase_terms(question,qt);rn=phrase_terms(record_text,rt);
    if(qn<2||rn<2)return 0;
    for(i=0;i<qn;++i){
        for(j=0;j<rn;++j){
            run=0;
            while(i+run<qn&&j+run<rn&&strcmp(qt[i+run],rt[j+run])==0)++run;
            if(run>best)best=run;
        }
    }
    return best>=2?300U+(unsigned int)(best-2)*100U:0U;
}
/* [AI:GPT-6 | 2026-10-08] Ground explicit quoted expression requests by their exact
 * multiword subject; isolated common terms cannot establish phrase evidence. */
static int quoted_expression(const char *request,char *out,size_t capacity)
{
    const char *start,*end;size_t length;
    if(request==NULL||out==NULL||capacity==0)return 0;
    start=strchr(request,'\'');
    if(start==NULL)start=strchr(request,'"');
    if(start==NULL)return 0;
    end=strchr(start+1,*start);
    if(end==NULL)return 0;
    length=(size_t)(end-start-1);
    if(length==0||length>=capacity)return 0;
    memcpy(out,start+1,length);out[length]='\0';
    return 1;
}
static int complete_phrase_match(const char *phrase,const char *record)
{
    char qt[TERM_COUNT][TERM_MAX],rt[TERM_COUNT][TERM_MAX];
    size_t qn,rn,i,j;
    memset(qt,0,sizeof(qt));memset(rt,0,sizeof(rt));
    qn=phrase_terms(phrase,qt);rn=phrase_terms(record,rt);
    if(qn<2||rn<qn)return 0;
    for(i=0;i+qn<=rn;++i){
        for(j=0;j<qn&&strcmp(qt[j],rt[i+j])==0;++j){}
        if(j==qn)return 1;
    }
    return 0;
}
static unsigned int record_score(const char *question,const corpus_record_t *record,unsigned int *matches_out){char qt[TERM_COUNT][TERM_MAX],rt[TERM_COUNT][TERM_MAX];size_t qn,rn,q;unsigned int matches=0,coverage=0,precision=0,score;int qr,rr;memset(qt,0,sizeof(qt));memset(rt,0,sizeof(rt));qn=terms(question,qt);rn=terms(record->text,rt);for(q=0;q<qn;++q)if(has_term(rt,rn,qt[q]))++matches;if(matches_out)*matches_out=matches;if(matches==0)return 0;if(qn>0)coverage=(unsigned int)((matches*200U)/qn);if(rn>0)precision=(unsigned int)((matches*100U)/rn);score=matches*100U+coverage+precision+lesson_bonus(question,record)+phrase_match_bonus(question,record->text);qr=reference_value(question);rr=reference_value(record->text);if(qr>0&&rr>0){if(qr==rr)score+=600U;else score=score>400U?score-400U:1U;}if(strcmp(record->category,"source")==0&&source_intent(question))score+=25U;return score;}
static size_t select_evidence(const char *question,const corpus_result_t *evidence,ranked_record_t selected[SELECTED_MAX]){ranked_record_t ranked[CORPUS_MAX];size_t count=evidence->count>CORPUS_MAX?CORPUS_MAX:evidence->count,i,j,out=0;int qr=reference_value(question);unsigned int reference_matches=0;for(i=0;i<count;++i){int rr;ranked[i].record=evidence->records[i];if(question_like_record(question,evidence->records[i].text)){ranked[i].score=0;ranked[i].matches=0;continue;}rr=reference_value(evidence->records[i].text);if(qr>0&&rr>0&&rr!=qr){ranked[i].score=0;ranked[i].matches=0;continue;}ranked[i].score=record_score(question,&evidence->records[i],&ranked[i].matches);if(qr>0&&rr==qr&&ranked[i].matches>0)++reference_matches;}for(i=1;i<count;++i){ranked_record_t key=ranked[i];j=i;while(j>0&&ranked[j-1].score<key.score){ranked[j]=ranked[j-1];--j;}ranked[j]=key;}if(qr>0){if(reference_matches==0)return 0;for(i=0;i<count&&out<SELECTED_MAX;++i){int rr=reference_value(ranked[i].record.text);if(ranked[i].matches>0&&rr==qr)selected[out++]=ranked[i];}for(i=0;i<count&&out<SELECTED_MAX;++i){int rr=reference_value(ranked[i].record.text);if(ranked[i].matches>0&&rr==0)selected[out++]=ranked[i];}return out;}for(i=0;i<count&&out<SELECTED_MAX;++i)if(ranked[i].score>0&&ranked[i].matches>0)selected[out++]=ranked[i];return out;}
static void trace_evidence(const char *question,const corpus_result_t *evidence,ranked_record_t selected[SELECTED_MAX],size_t selected_count){size_t i,count;if(response_host==NULL||response_host->send_message==NULL||question==NULL||evidence==NULL)return;count=evidence->count>CORPUS_MAX?CORPUS_MAX:evidence->count;{char message[512];snprintf(message,sizeof(message),"[RESPONSE] evidence trace query=\"%.320s\" retrieved=%zu selected=%zu reference=%d",question,count,selected_count,reference_value(question));(void)response_host->send_message(message);}for(i=0;i<count;++i){unsigned int matches=0,score=record_score(question,&evidence->records[i],&matches);char message[768];snprintf(message,sizeof(message),"[RESPONSE] evidence candidate %zu score=%u matches=%u reference=%d category=%.48s source=%.160s text=\"%.320s\"",i+1,score,matches,reference_value(evidence->records[i].text),evidence->records[i].category,evidence->records[i].source,evidence->records[i].text);(void)response_host->send_message(message);}for(i=0;i<selected_count;++i){char message[768];snprintf(message,sizeof(message),"[RESPONSE] evidence selected %zu score=%u matches=%u reference=%d category=%.48s source=%.160s text=\"%.320s\"",i+1,selected[i].score,selected[i].matches,reference_value(selected[i].record.text),selected[i].record.category,selected[i].record.source,selected[i].record.text);(void)response_host->send_message(message);}}
/* Lesson entry identifiers are retrieval metadata, not user-facing prose. */
static const char *presentation_text(const corpus_record_t *record)
{
    const char *p;const char *start;
    if(record==NULL)return "";
    p=record->text;
    if(strncmp(record->source,"lesson:",7)!=0||strncmp(p,"ENTRY ",6)!=0)return p;
    start=p+6;
    if(!isdigit((unsigned char)*start))return p;
    while(isdigit((unsigned char)*start))++start;
    if(*start!=':')return p;
    ++start;
    while(*start==' '||*start=='\t')++start;
    return start;
}
static void grounded_fallback(const char *question,ranked_record_t selected[SELECTED_MAX],size_t selected_count,digit_response_result_t *output){size_t i,offset=0;int show_evidence=evidence_requested(question);output->answered=1;if(selected_count==0){snprintf(output->answer,sizeof(output->answer),"I don't have enough information to answer that.");return;}if(!show_evidence){snprintf(output->answer,sizeof(output->answer),"%s",presentation_text(&selected[0].record));return;}snprintf(output->answer,sizeof(output->answer),"Evidence: ");offset=strlen(output->answer);for(i=0;i<selected_count&&i<3;++i){int written;if(strcmp(selected[i].record.category,"source")==0)written=snprintf(output->answer+offset,sizeof(output->answer)-offset,"%s[%s] %s",i?" ":"",selected[i].record.source,selected[i].record.text);else written=snprintf(output->answer+offset,sizeof(output->answer)-offset,"%s%s",i?" ":"",presentation_text(&selected[i].record));if(written<=0||(size_t)written>=sizeof(output->answer)-offset)break;offset+=(size_t)written;}}
static int validate_inbound(const char *raw,char *normalized,size_t normalized_size){validator_inbound_request_t request;validator_inbound_result_t result;stnlabz_module_result_t status;size_t used=0;if(raw==NULL||normalized==NULL||normalized_size==0||response_host==NULL||response_host->invoke_service==NULL)return 0;memset(&request,0,sizeof(request));memset(&result,0,sizeof(result));snprintf(request.raw,sizeof(request.raw),"%s",raw);status=response_host->invoke_service(VALIDATOR_INBOUND_SERVICE,&request,sizeof(request),&result,sizeof(result),&used);if(status!=STNLABZ_MODULE_OK||used!=sizeof(result)||result.status!=VALIDATOR_PASS||result.normalized[0]=='\0')return 0;snprintf(normalized,normalized_size,"%s",result.normalized);return 1;}
static void validator_evidence(ranked_record_t selected[SELECTED_MAX],size_t selected_count,char *buffer,size_t buffer_size){size_t i,offset=0;if(buffer==NULL||buffer_size==0)return;buffer[0]='\0';for(i=0;i<selected_count&&offset+1<buffer_size;++i){int written=snprintf(buffer+offset,buffer_size-offset,"%s%s",i?"\n":"",selected[i].record.text);if(written<=0||(size_t)written>=buffer_size-offset)break;offset+=(size_t)written;}}
static int validate_outbound_evidence(const char *raw,const char *normalized,const char *candidate,const char *evidence,unsigned int attempt,validator_outbound_result_t *validation){validator_outbound_request_t request;stnlabz_module_result_t status;size_t used=0;if(raw==NULL||normalized==NULL||candidate==NULL||validation==NULL||response_host==NULL||response_host->invoke_service==NULL)return 0;memset(&request,0,sizeof(request));memset(validation,0,sizeof(*validation));snprintf(request.raw,sizeof(request.raw),"%s",raw);snprintf(request.normalized,sizeof(request.normalized),"%s",normalized);snprintf(request.candidate,sizeof(request.candidate),"%s",candidate);if(evidence!=NULL)snprintf(request.evidence,sizeof(request.evidence),"%s",evidence);request.attempt=attempt;status=response_host->invoke_service(VALIDATOR_OUTBOUND_SERVICE,&request,sizeof(request),validation,sizeof(*validation),&used);return status==STNLABZ_MODULE_OK&&used==sizeof(*validation);}
static int validate_outbound(const char *raw,const char *normalized,const char *candidate,ranked_record_t selected[SELECTED_MAX],size_t selected_count,unsigned int attempt,validator_outbound_result_t *validation){char evidence[VALIDATOR_TEXT_MAX];memset(evidence,0,sizeof(evidence));validator_evidence(selected,selected_count,evidence,sizeof(evidence));return validate_outbound_evidence(raw,normalized,candidate,evidence,attempt,validation);}
static stnlabz_module_result_t answer_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *handler_context){const digit_response_request_t *input=request;digit_response_result_t output;corpus_result_t evidence;ranked_record_t selected[SELECTED_MAX];validator_outbound_result_t validation;response_intent_t intent;char normalized[VALIDATOR_TEXT_MAX];char rendered_evidence[VALIDATOR_TEXT_MAX];char phrase[DIGIT_RESPONSE_QUESTION_MAX];const char *query;size_t selected_count;(void)handler_context;if(request==NULL||request_size!=sizeof(*input)||response==NULL||response_used==NULL||response_size<sizeof(output))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;if(memchr(input->question,'\0',sizeof(input->question))==NULL||input->question[0]=='\0')return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;if(response_host==NULL||response_host->invoke_service==NULL)return STNLABZ_MODULE_ERR_START_FAILED;memset(&output,0,sizeof(output));memset(&intent,0,sizeof(intent));memset(normalized,0,sizeof(normalized));memset(rendered_evidence,0,sizeof(rendered_evidence));parse_intent_envelope(input->question,&intent);if(!validate_inbound(intent.request,normalized,sizeof(normalized))){output.answered=1;snprintf(output.answer,sizeof(output.answer),"I couldn't validate that request well enough to answer it safely.");memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}/* [AI:GPT-6 | 2026-10-08] Dispatcher wraps greetings in a structured CONVERSATION envelope; recognize the normalized request before corpus and evidence validation. */if((!intent.structured||strcmp(intent.intent,"CONVERSATION")==0)&&conversational_greeting(normalized,output.answer,sizeof(output.answer))){output.answered=1;memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}/* Bounded refusal rather than a wrong numeric answer from lexical evidence. */
 if(arithmetic_question(normalized)){
  output.answered=1;
  if(!arithmetic_calculate(normalized,output.answer,sizeof(output.answer)))
   snprintf(output.answer,sizeof(output.answer),
    "I cannot evaluate that arithmetic expression within the supported two-operand integer limits.");
  memcpy(response,&output,sizeof(output));*response_used=sizeof(output);
  return STNLABZ_MODULE_OK;
 }
 memset(&evidence,0,sizeof(evidence));memset(selected,0,sizeof(selected));query=intent.structured?retrieval_text(&intent):normalized;
    /* A quoted subject in an explicit knowledge request takes precedence over
     * framing instructions such as "in your own words". */
    {
        int phrase_required=intent.structured &&
            (strcmp(intent.intent,"EXPLAIN")==0||strcmp(intent.intent,"DEFINE")==0) &&
            quoted_expression(intent.request,phrase,sizeof(phrase)) &&
            phrase_terms(phrase,(char [TERM_COUNT][TERM_MAX]){{0}})>=2;
        if(phrase_required)query=phrase;
        if(!collect_corpus(query,&evidence)){output.answered=1;snprintf(output.answer,sizeof(output.answer),"I can't access retained information right now.");memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}collect_source(query,&evidence);selected_count=select_evidence(query,&evidence,selected);
        if(phrase_required){
            size_t i,kept=0;
            for(i=0;i<selected_count;++i)
                if(complete_phrase_match(phrase,selected[i].record.text))
                    selected[kept++]=selected[i];
            selected_count=kept;
        }
        trace_evidence(query,&evidence,selected,selected_count);
    }
    /* [AI:GPT-6 | 2026-10-09] An evidence-free factual request must
     * return an explicit uncertainty result, not a candidate submitted as
     * if supported by retained records. Comparisons likewise need evidence. */
    if(selected_count==0){
        output.answered=1;
        snprintf(output.answer,sizeof(output.answer),
            "I don't have enough grounded information to answer that.");
        memcpy(response,&output,sizeof(output));
        *response_used=sizeof(output);
        return STNLABZ_MODULE_OK;
    }
    output.evidence_count=(unsigned int)selected_count;render_intent_answer(&intent,query,selected,selected_count,&output,rendered_evidence,sizeof(rendered_evidence));if(response_host->send_message){char diagnostic[512];snprintf(diagnostic,sizeof(diagnostic),"[RESPONSE] outbound candidate intent=%.24s subject=%.80s text=\"%.300s\"",intent.intent,query,output.answer);(void)response_host->send_message(diagnostic);}if(!(intent.structured&&strcmp(intent.intent,"COMPARE")==0?validate_outbound_evidence(input->question,normalized,output.answer,rendered_evidence,1U,&validation):validate_outbound(input->question,normalized,output.answer,selected,selected_count,1U,&validation))){if(response_host->send_message)(void)response_host->send_message("[RESPONSE] Validator unavailable; deterministic response blocked");output.answered=1;snprintf(output.answer,sizeof(output.answer),"I couldn't validate the response.");}else if(validation.status!=VALIDATOR_PASS){if(response_host->send_message){char message[384];snprintf(message,sizeof(message),"[RESPONSE] Validator rejected deterministic response: %s",validation.reason[0]?validation.reason:"candidate rejected");(void)response_host->send_message(message);}output.answered=1;snprintf(output.answer,sizeof(output.answer),"I couldn't produce a grounded answer that passed validation.");}memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}
/* [AI:GPT-6 | 2026-10-09] Executed deterministic checks.
 * Networked Corpus/Reasoning/Validator acceptance remains separate. */
static stnlabz_module_result_t response_qualify(stnlabz_module_qualification_result_t *result){
 char words[TERM_COUNT][TERM_MAX]={{0}},answer[128]={0},phrase[128]={0};
 response_intent_t parsed;
 unsigned passed=0;
 if(!result)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
 memset(result,0,sizeof(*result));
 passed+=(unsigned)(terms("Compare C and Python",words)==3);
 passed+=(unsigned)stopword("what");
 passed+=(unsigned)!stopword("compiler");
 passed+=(unsigned)conversational_greeting("hello Digit",answer,sizeof(answer))&&strcmp(answer,"Hello.")==0;
 passed+=(unsigned)!conversational_greeting("who are you?",answer,sizeof(answer));
 parse_intent_envelope("INTENT: COMPARE\nTARGET: KNOWLEDGE\nSUBJECT: C and Python\nREQUEST: Compare C and Python",&parsed);
 passed+=(unsigned)parsed.structured&&strcmp(parsed.intent,"COMPARE")==0;
 passed+=(unsigned)strcmp(retrieval_text(&parsed),"C and Python")==0;
 passed+=(unsigned)quoted_expression("explain 'running on fumes'",phrase,sizeof(phrase))&&strcmp(phrase,"running on fumes")==0;
 passed+=(unsigned)complete_phrase_match("running on fumes","A lesson about running on fumes today");
 passed+=(unsigned)!question_like_record("what is your mission?","Unrelated module build status");
 passed+=(unsigned)arithmetic_question("What is 2 plus 2?");
 passed+=(unsigned)!arithmetic_question("Compare C and Python");
 result->tests_executed=12;result->tests_passed=passed;
 result->tests_failed=result->tests_executed-passed;
 result->negative_test_executed=1;
 result->negative_test_passed=!quoted_expression("unterminated 'quote",phrase,sizeof(phrase));
 return result->tests_failed||!result->negative_test_passed?
   STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}
static stnlabz_module_result_t response_start(const stnlabz_module_host_t *host){if(host==NULL||host->register_service==NULL||host->invoke_service==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;if(!host->register_service(DIGIT_RESPONSE_SERVICE,answer_service,NULL))return STNLABZ_MODULE_ERR_START_FAILED;response_host=host;if(host->send_message)(void)host->send_message("[RESPONSE] module active: grounded retained-knowledge response registered");return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t response_stop(void){if(response_host!=NULL&&response_host->unregister_service!=NULL)if(!response_host->unregister_service(DIGIT_RESPONSE_SERVICE,NULL))return STNLABZ_MODULE_ERR_STOP_FAILED;response_host=NULL;return STNLABZ_MODULE_OK;}
static const stnlabz_module_descriptor_t response_descriptor={"response","Digit Response",1,7,5,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,response_qualify,response_start,response_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &response_descriptor;}
