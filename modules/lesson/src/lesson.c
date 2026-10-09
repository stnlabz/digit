#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lesson.h"
#include "corpus.h"

#define DIGIT_LESSON_ROOT "/opt/digit/lessons"
#define LESSON_LINE_MAX DIGIT_CORPUS_TEXT_MAX
#define LESSON_CATEGORY "OPERATOR_LEARNED"

static const stnlabz_module_host_t *lesson_host = NULL;

static int safe_name(const char *name)
{
    size_t i;
    if(name==NULL||name[0]=='\0')return 0;
    for(i=0;name[i]!='\0';++i)
        if(!(isalnum((unsigned char)name[i])||name[i]=='_'||name[i]=='-'))return 0;
    return 1;
}

static char *trim(char *text)
{
    char *end;
    while(*text&&isspace((unsigned char)*text))++text;
    if(*text=='\0')return text;
    end=text+strlen(text)-1;
    while(end>text&&isspace((unsigned char)*end))*end--='\0';
    return text;
}

static uint64_t hash_text(uint64_t hash,const char *text)
{
    const unsigned char *p=(const unsigned char *)text;
    while(*p!=0){hash^=(uint64_t)*p++;hash*=UINT64_C(1099511628211);}
    return hash;
}

static void build_record_id(const char *source,const char *text,char *output,size_t output_size)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    hash=hash_text(hash,source);
    hash=hash_text(hash,LESSON_CATEGORY);
    hash=hash_text(hash,text);
    snprintf(output,output_size,"DIGIT-%016llx",(unsigned long long)hash);
}

static int store_unit(const char *source,const char *unit)
{
    digit_corpus_record_t record;
    digit_corpus_contains_result_t contains;
    digit_corpus_append_result_t append;
    size_t used=0;
    stnlabz_module_result_t result;

    memset(&record,0,sizeof(record));
    snprintf(record.category,sizeof(record.category),"%s",LESSON_CATEGORY);
    snprintf(record.source,sizeof(record.source),"%s",source);
    snprintf(record.text,sizeof(record.text),"%s",unit);
    build_record_id(source,unit,record.id,sizeof(record.id));

    memset(&contains,0,sizeof(contains));
    result=lesson_host->invoke_service(DIGIT_CORPUS_CONTAINS_SERVICE,record.id,strlen(record.id)+1,&contains,sizeof(contains),&used);
    if(result!=STNLABZ_MODULE_OK||used!=sizeof(contains))return 0;
    if(contains.contains)return 1;

    memset(&append,0,sizeof(append));used=0;
    result=lesson_host->invoke_service(DIGIT_CORPUS_APPEND_SERVICE,&record,sizeof(record),&append,sizeof(append),&used);
    return result==STNLABZ_MODULE_OK&&used==sizeof(append)&&append.appended;
}

static stnlabz_module_result_t lesson_ingest_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *handler_context)
{
    const digit_lesson_ingest_request_t *input=request;
    digit_lesson_ingest_result_t output;
    FILE *file;
    char line[LESSON_LINE_MAX];
    char source[DIGIT_CORPUS_SOURCE_MAX];
    (void)handler_context;

    if(request==NULL||request_size!=sizeof(*input)||response==NULL||response_used==NULL||response_size<sizeof(output))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(memchr(input->name,'\0',sizeof(input->name))==NULL||!safe_name(input->name))return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(lesson_host==NULL||lesson_host->invoke_service==NULL)return STNLABZ_MODULE_ERR_START_FAILED;

    memset(&output,0,sizeof(output));
    snprintf(output.path,sizeof(output.path),"%s/%s.txt",DIGIT_LESSON_ROOT,input->name);
    file=fopen(output.path,"r");
    if(file==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    snprintf(source,sizeof(source),"lesson:%s.txt",input->name);

    while(fgets(line,sizeof(line),file)!=NULL)
    {
        char *unit=trim(line);
        if(unit[0]=='\0'||unit[0]=='#')continue;
        ++output.units;
        if(store_unit(source,unit))++output.stored;
        else ++output.rejected;
    }

    fclose(file);
    output.completed=1;
    memcpy(response,&output,sizeof(output));
    *response_used=sizeof(output);
    if(lesson_host->send_message!=NULL)
    {
        char message[256];
        snprintf(message,sizeof(message),"[LESSON] ingested %s: units=%zu stored=%zu rejected=%zu",input->name,output.units,output.stored,output.rejected);
        (void)lesson_host->send_message(message);
    }
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t lesson_qualify(stnlabz_module_qualification_result_t *result)
{
    /* [AI:GPT-6 | 2026-10-09] Execute deterministic qualification
     * without touching production lesson files or Corpus services. */
    static const struct { const char *name; int allowed; } cases[]={
        {"lesson-one",1},{"unit_2",1},{"A3",1},
        {"../unsafe",0},{"two words",0},{"",0},
        {"lesson.txt",0},{"a/b",0},{"name\\\\other",0},
        {"one:two",0}
    };
    size_t i;unsigned int passed=0;
    if(result==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result,0,sizeof(*result));
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i)
        if(safe_name(cases[i].name)==cases[i].allowed)++passed;
    result->tests_executed=(unsigned int)(sizeof(cases)/sizeof(cases[0]));
    result->tests_passed=passed;
    result->tests_failed=result->tests_executed-result->tests_passed;
    result->negative_test_executed=1;
    result->negative_test_passed=
      lesson_ingest_service(NULL,0,NULL,0,NULL,NULL)==STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    return result->tests_failed||!result->negative_test_passed?
      STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t lesson_start(const stnlabz_module_host_t *host)
{
    if(host==NULL||host->register_service==NULL||host->invoke_service==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!host->register_service(DIGIT_LESSON_INGEST_SERVICE,lesson_ingest_service,NULL))return STNLABZ_MODULE_ERR_START_FAILED;
    lesson_host=host;
    if(host->send_message!=NULL)(void)host->send_message("[LESSON] module active: deterministic text lesson ingestion registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t lesson_stop(void)
{
    if(lesson_host!=NULL&&lesson_host->unregister_service!=NULL)
        if(!lesson_host->unregister_service(DIGIT_LESSON_INGEST_SERVICE,NULL))return STNLABZ_MODULE_ERR_STOP_FAILED;
    lesson_host=NULL;return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t lesson_descriptor={"lesson","Digit Lesson",1,1,1,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,lesson_qualify,lesson_start,lesson_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &lesson_descriptor;}
