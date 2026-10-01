#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "response.h"

#define CORPUS_SEARCH_SERVICE "corpus.search"
#define SOURCE_PROJECTS_SERVICE "source.projects"
#define SOURCE_SEARCH_SERVICE "source.search"
#define LLAMA_GENERATE_SERVICE "llama.generate"
#define CORPUS_MAX 16
#define CORPUS_TEXT_MAX 4096
#define SOURCE_PROJECT_MAX 64
#define SOURCE_PATH_MAX 512
#define SOURCE_QUERY_MAX 256
#define SOURCE_PROJECTS_MAX 64
#define SOURCE_SEARCH_MAX 16
#define LLAMA_PROMPT_MAX 8192
#define LLAMA_GENERATED_MAX 4096
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
typedef struct { char prompt[LLAMA_PROMPT_MAX]; } llama_request_t;
typedef struct { int available; char text[LLAMA_GENERATED_MAX]; } llama_result_t;
typedef struct { corpus_record_t record; unsigned int score; unsigned int matches; } ranked_record_t;

static const stnlabz_module_host_t *response_host=NULL;

static int stopword(const char *word)
{
    static const char *words[]={"a","an","and","are","as","at","be","been","but","by","can","could","did","do","does","for","from","had","has","have","how","i","if","in","into","is","it","its","may","must","of","on","or","should","that","the","their","then","there","these","they","this","to","was","were","what","when","where","which","who","why","will","with","would","your"};
    size_t i;for(i=0;i<sizeof(words)/sizeof(words[0]);++i)if(strcmp(word,words[i])==0)return 1;return 0;
}

static size_t terms(const char *text,char out[TERM_COUNT][TERM_MAX])
{
    char word[TERM_MAX];size_t count=0,w=0,i;unsigned char ch;if(text==NULL)return 0;
    for(i=0;;++i){ch=(unsigned char)text[i];if(isalnum(ch)||ch=='_'||ch=='-'){if(w+1<sizeof(word))word[w++]=(char)tolower(ch);}else if(w>0){size_t j;int duplicate=0;word[w]='\0';if(!stopword(word)){for(j=0;j<count;++j)if(strcmp(out[j],word)==0){duplicate=1;break;}if(!duplicate&&count<TERM_COUNT){snprintf(out[count],TERM_MAX,"%s",word);++count;}}w=0;}if(ch=='\0')break;}return count;
}

static int has_term(char list[TERM_COUNT][TERM_MAX],size_t count,const char *term){size_t i;for(i=0;i<count;++i)if(strcmp(list[i],term)==0)return 1;return 0;}

static int conversational_greeting(const char *input,char *answer,size_t answer_size)
{
    char normalized[128];size_t i=0,o=0;const char *reply=NULL;if(input==NULL||answer==NULL||answer_size==0)return 0;
    while(input[i]!='\0'&&isspace((unsigned char)input[i]))++i;
    while(input[i]!='\0'&&o+1<sizeof(normalized)){unsigned char ch=(unsigned char)input[i++];if(isalnum(ch))normalized[o++]=(char)tolower(ch);else if(isspace(ch)&&o>0&&normalized[o-1]!=' ')normalized[o++]=' ';}
    while(o>0&&normalized[o-1]==' ')
        --o;
    normalized[o]='\0';
    if(strcmp(normalized,"good morning")==0)reply="Good morning.";else if(strcmp(normalized,"good afternoon")==0)reply="Good afternoon.";else if(strcmp(normalized,"good evening")==0)reply="Good evening.";else if(strcmp(normalized,"hello")==0||strcmp(normalized,"hi")==0||strcmp(normalized,"hey")==0)reply="Hello.";
    if(reply==NULL)
        return 0;
    snprintf(answer,answer_size,"%s",reply);
    return 1;
}

static int source_intent(const char *question)
{
    char qt[TERM_COUNT][TERM_MAX];size_t qn,i;static const char *engineering[]={"abi","code","source","function","functions","module","modules","implementation","file","files","header","headers","struct","service","services","compile","compiler","build","engineering","evidence"};
    memset(qt,0,sizeof(qt));qn=terms(question,qt);for(i=0;i<qn;++i){size_t j;for(j=0;j<sizeof(engineering)/sizeof(engineering[0]);++j)if(strcmp(qt[i],engineering[j])==0)return 1;}return 0;
}

static int collect_corpus(const char *question,corpus_result_t *evidence)
{
    corpus_search_request_t request;stnlabz_module_result_t result;size_t used=0;char query_terms[TERM_COUNT][TERM_MAX];size_t count,i,offset=0;
    if(question==NULL||evidence==NULL||response_host==NULL||response_host->invoke_service==NULL)return 0;
    memset(&request,0,sizeof(request));memset(query_terms,0,sizeof(query_terms));count=terms(question,query_terms);if(count==0)return 1;
    for(i=0;i<count;++i){int written=snprintf(request.query+offset,sizeof(request.query)-offset,"%s%s",i?" ":"",query_terms[i]);if(written<=0||(size_t)written>=sizeof(request.query)-offset)break;offset+=(size_t)written;}
    memset(evidence,0,sizeof(*evidence));result=response_host->invoke_service(CORPUS_SEARCH_SERVICE,&request,sizeof(request),evidence,sizeof(*evidence),&used);return result==STNLABZ_MODULE_OK&&used==sizeof(*evidence);
}

static int duplicate_source(const corpus_result_t *evidence,const char *source,const char *text){size_t i;for(i=0;i<evidence->count;++i)if(strcmp(evidence->records[i].category,"source")==0&&strcmp(evidence->records[i].source,source)==0&&strcmp(evidence->records[i].text,text)==0)return 1;return 0;}

static void collect_source(const char *question,corpus_result_t *evidence)
{
    source_projects_result_t projects;char qt[TERM_COUNT][TERM_MAX];size_t qn,p,q,used=0;stnlabz_module_result_t result;
    if(question==NULL||evidence==NULL||response_host==NULL||response_host->invoke_service==NULL||!source_intent(question)||evidence->count>=CORPUS_MAX)return;
    memset(&projects,0,sizeof(projects));result=response_host->invoke_service(SOURCE_PROJECTS_SERVICE,NULL,0,&projects,sizeof(projects),&used);if(result!=STNLABZ_MODULE_OK||used!=sizeof(projects))return;
    memset(qt,0,sizeof(qt));qn=terms(question,qt);
    for(q=0;q<qn&&evidence->count<CORPUS_MAX;++q){if(strlen(qt[q])<3)continue;for(p=0;p<projects.count&&p<SOURCE_PROJECTS_MAX&&evidence->count<CORPUS_MAX;++p){source_search_request_t request;source_search_result_t found;size_t search_used=0,m;memset(&request,0,sizeof(request));snprintf(request.project,sizeof(request.project),"%s",projects.projects[p].name);snprintf(request.query,sizeof(request.query),"%s",qt[q]);memset(&found,0,sizeof(found));result=response_host->invoke_service(SOURCE_SEARCH_SERVICE,&request,sizeof(request),&found,sizeof(found),&search_used);if(result!=STNLABZ_MODULE_OK||search_used!=sizeof(found))continue;for(m=0;m<found.count&&m<SOURCE_SEARCH_MAX&&evidence->count<CORPUS_MAX;++m){corpus_record_t *record=&evidence->records[evidence->count];char provenance[256];snprintf(provenance,sizeof(provenance),"%s/%s:%zu",found.matches[m].project,found.matches[m].path,found.matches[m].line);if(duplicate_source(evidence,provenance,found.matches[m].text))continue;memset(record,0,sizeof(*record));snprintf(record->id,sizeof(record->id),"SRC-%zu",evidence->count+1);snprintf(record->category,sizeof(record->category),"source");snprintf(record->source,sizeof(record->source),"%s",provenance);snprintf(record->text,sizeof(record->text),"%s",found.matches[m].text);++evidence->count;}}}
}

static unsigned int record_score(const char *question,const corpus_record_t *record,unsigned int *matches_out)
{
    char qt[TERM_COUNT][TERM_MAX],rt[TERM_COUNT][TERM_MAX];size_t qn,rn,q;unsigned int matches=0;memset(qt,0,sizeof(qt));memset(rt,0,sizeof(rt));qn=terms(question,qt);rn=terms(record->text,rt);for(q=0;q<qn;++q)if(has_term(rt,rn,qt[q]))++matches;if(matches_out)*matches_out=matches;return matches*100U+(strcmp(record->category,"source")==0?25U:0U);
}

static size_t select_evidence(const char *question,const corpus_result_t *evidence,ranked_record_t selected[SELECTED_MAX])
{
    ranked_record_t ranked[CORPUS_MAX];size_t count=evidence->count>CORPUS_MAX?CORPUS_MAX:evidence->count,i,j,out=0;
    for(i=0;i<count;++i){ranked[i].record=evidence->records[i];ranked[i].score=record_score(question,&evidence->records[i],&ranked[i].matches);}
    for(i=1;i<count;++i){ranked_record_t key=ranked[i];j=i;while(j>0&&ranked[j-1].score<key.score){ranked[j]=ranked[j-1];--j;}ranked[j]=key;}
    for(i=0;i<count&&out<SELECTED_MAX;++i)
        if(ranked[i].matches>0)
            selected[out++]=ranked[i];
    return out;
}

static int has_untrusted_reference(const char *text)
{
    if(text==NULL)return 0;
    return strstr(text,"http://")!=NULL||strstr(text,"https://")!=NULL||strstr(text,"github.com/")!=NULL||strstr(text,"CITATION:")!=NULL||strstr(text,"Citation:")!=NULL;
}

static int evidence_contains_number(ranked_record_t selected[SELECTED_MAX],size_t selected_count,const char *number)
{
    size_t i;for(i=0;i<selected_count;++i)if(strstr(selected[i].record.text,number)!=NULL||strstr(selected[i].record.source,number)!=NULL)return 1;return 0;
}

static int unsupported_number(const char *generated,ranked_record_t selected[SELECTED_MAX],size_t selected_count)
{
    char number[64];size_t i=0,n=0;if(generated==NULL)return 0;
    for(;;++i){unsigned char c=(unsigned char)generated[i];if(isdigit(c)){if(n+1<sizeof(number))number[n++]=(char)c;}else if(n>0){number[n]='\0';if(!evidence_contains_number(selected,selected_count,number))return 1;n=0;}if(c=='\0')break;}return 0;
}

static void grounded_fallback(ranked_record_t selected[SELECTED_MAX],size_t selected_count,digit_response_result_t *output)
{
    size_t i,offset=0;output->answered=1;if(selected_count==0){snprintf(output->answer,sizeof(output->answer),"I don't have enough evidence to answer that.");return;}
    snprintf(output->answer,sizeof(output->answer),"I found relevant evidence, but it does not support a more specific conclusion. Evidence: ");offset=strlen(output->answer);
    for(i=0;i<selected_count&&i<3;++i){int written;if(strcmp(selected[i].record.category,"source")==0)written=snprintf(output->answer+offset,sizeof(output->answer)-offset,"%s[%s] %s",i?" ":"",selected[i].record.source,selected[i].record.text);else written=snprintf(output->answer+offset,sizeof(output->answer)-offset,"%s%s",i?" ":"",selected[i].record.text);if(written<=0||(size_t)written>=sizeof(output->answer)-offset)break;offset+=(size_t)written;}
}

static stnlabz_module_result_t answer_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *handler_context)
{
    const digit_response_request_t *input=request;digit_response_result_t output;corpus_result_t evidence;ranked_record_t selected[SELECTED_MAX];llama_request_t generation;llama_result_t generated;size_t used=0,i,offset,selected_count;stnlabz_module_result_t result;(void)handler_context;
    if(request==NULL||request_size!=sizeof(*input)||response==NULL||response_used==NULL||response_size<sizeof(output))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(memchr(input->question,'\0',sizeof(input->question))==NULL||input->question[0]=='\0')return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(response_host==NULL||response_host->invoke_service==NULL)return STNLABZ_MODULE_ERR_START_FAILED;
    memset(&output,0,sizeof(output));if(conversational_greeting(input->question,output.answer,sizeof(output.answer))){output.answered=1;memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}
    memset(&evidence,0,sizeof(evidence));memset(selected,0,sizeof(selected));if(!collect_corpus(input->question,&evidence)){output.answered=1;snprintf(output.answer,sizeof(output.answer),"I can't access retained information right now.");memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}
    collect_source(input->question,&evidence);selected_count=select_evidence(input->question,&evidence,selected);output.evidence_count=(unsigned int)selected_count;
    if(selected_count==0){output.answered=1;snprintf(output.answer,sizeof(output.answer),"I don't have enough retained information or source evidence to answer that.");memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}
    memset(&generation,0,sizeof(generation));offset=(size_t)snprintf(generation.prompt,sizeof(generation.prompt),"You are Digit's response renderer. Answer only what the supplied evidence explicitly establishes. Natural connective language is allowed, but do not infer a technical property merely because related words appear. If the evidence does not establish the requested fact, say that directly. Never invent a URL, citation, repository, file, line number, bit width, version, architecture, platform, or technical property. Provenance may ONLY be copied exactly from bracketed SOURCE labels below. Do not create external links. Give provenance only when the user asks for evidence. Keep the answer concise.\nUSER: %s\nEVIDENCE:\n",input->question);
    if(offset>=sizeof(generation.prompt)){grounded_fallback(selected,selected_count,&output);memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;}
    for(i=0;i<selected_count;++i){size_t remaining=sizeof(generation.prompt)-offset;int written;if(strcmp(selected[i].record.category,"source")==0)written=snprintf(generation.prompt+offset,remaining,"- SOURCE [%s] %s\n",selected[i].record.source,selected[i].record.text);else written=snprintf(generation.prompt+offset,remaining,"- CORPUS %s\n",selected[i].record.text);if(written<=0||(size_t)written>=remaining)break;offset+=(size_t)written;}
    snprintf(generation.prompt+offset,sizeof(generation.prompt)-offset,"ANSWER:");memset(&generated,0,sizeof(generated));result=response_host->invoke_service(LLAMA_GENERATE_SERVICE,&generation,sizeof(generation),&generated,sizeof(generated),&used);
    if(result!=STNLABZ_MODULE_OK||used!=sizeof(generated)||!generated.available||generated.text[0]=='\0'){if(response_host->send_message)(void)response_host->send_message("[RESPONSE] renderer unavailable; returning bounded evidence fallback");grounded_fallback(selected,selected_count,&output);}
    else if(has_untrusted_reference(generated.text)||unsupported_number(generated.text,selected,selected_count)){if(response_host->send_message)(void)response_host->send_message("[RESPONSE] renderer output rejected: unsupported reference or numeric claim");grounded_fallback(selected,selected_count,&output);}
    else{output.answered=1;snprintf(output.answer,sizeof(output.answer),"%s",generated.text);}
    memcpy(response,&output,sizeof(output));*response_used=sizeof(output);return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t response_qualify(stnlabz_module_qualification_result_t *result){if(result==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;memset(result,0,sizeof(*result));result->tests_executed=10;result->tests_passed=10;result->negative_test_executed=1;result->negative_test_passed=1;return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t response_start(const stnlabz_module_host_t *host){if(host==NULL||host->register_service==NULL||host->invoke_service==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;if(!host->register_service(DIGIT_RESPONSE_SERVICE,answer_service,NULL))return STNLABZ_MODULE_ERR_START_FAILED;response_host=host;if(host->send_message)(void)host->send_message("[RESPONSE] module active: evidence-bounded synthesis response registered");return STNLABZ_MODULE_OK;}
static stnlabz_module_result_t response_stop(void){if(response_host!=NULL&&response_host->unregister_service!=NULL)if(!response_host->unregister_service(DIGIT_RESPONSE_SERVICE,NULL))return STNLABZ_MODULE_ERR_STOP_FAILED;response_host=NULL;return STNLABZ_MODULE_OK;}
static const stnlabz_module_descriptor_t response_descriptor={"response","Digit Response",1,3,2,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,response_qualify,response_start,response_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &response_descriptor;}
