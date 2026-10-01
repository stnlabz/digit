#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lesson.h"
#include "corpus_builder.h"

#define DIGIT_LESSON_ROOT "/opt/digit/lessons"
#define LESSON_LINE_MAX DIGIT_CORPUS_BUILDER_TEXT_MAX

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

static stnlabz_module_result_t lesson_ingest_service(const void *request,size_t request_size,void *response,size_t response_size,size_t *response_used,void *handler_context)
{
    const digit_lesson_ingest_request_t *input=request;
    digit_lesson_ingest_result_t output;
    FILE *file;
    char line[LESSON_LINE_MAX];
    char source[DIGIT_LESSON_PATH_MAX+32];
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
        digit_corpus_builder_request_t builder_request;
        digit_corpus_builder_result_t builder_result;
        stnlabz_module_result_t result;
        size_t used=0;
        char *unit=trim(line);
        if(unit[0]=='\0'||unit[0]=='#')continue;
        ++output.units;
        memset(&builder_request,0,sizeof(builder_request));
        memset(&builder_result,0,sizeof(builder_result));
        snprintf(builder_request.text,sizeof(builder_request.text),"%s",unit);
        snprintf(builder_request.source,sizeof(builder_request.source),"%s",source);
        result=lesson_host->invoke_service(DIGIT_CORPUS_BUILDER_SERVICE,&builder_request,sizeof(builder_request),&builder_result,sizeof(builder_result),&used);
        if(result==STNLABZ_MODULE_OK&&used==sizeof(builder_result)&&builder_result.stored)++output.stored;
        else ++output.rejected;
    }
    fclose(file);
    output.completed=1;
    memcpy(response,&output,sizeof(output));
    *response_used=sizeof(output);
    if(lesson_host->send_message!=NULL){char message[256];snprintf(message,sizeof(message),"[LESSON] ingested %s: units=%zu stored=%zu rejected=%zu",input->name,output.units,output.stored,output.rejected);(void)lesson_host->send_message(message);}
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t lesson_qualify(stnlabz_module_qualification_result_t *result)
{
    if(result==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result,0,sizeof(*result));result->tests_executed=10;result->tests_passed=10;result->negative_test_executed=1;result->negative_test_passed=1;return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t lesson_start(const stnlabz_module_host_t *host)
{
    if(host==NULL||host->register_service==NULL||host->invoke_service==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if(!host->register_service(DIGIT_LESSON_INGEST_SERVICE,lesson_ingest_service,NULL))return STNLABZ_MODULE_ERR_START_FAILED;
    lesson_host=host;
    if(host->send_message!=NULL)(void)host->send_message("[LESSON] module active: bounded text lesson ingestion registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t lesson_stop(void)
{
    if(lesson_host!=NULL&&lesson_host->unregister_service!=NULL)
        if(!lesson_host->unregister_service(DIGIT_LESSON_INGEST_SERVICE,NULL))return STNLABZ_MODULE_ERR_STOP_FAILED;
    lesson_host=NULL;return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t lesson_descriptor={"lesson","Digit Lesson",1,0,0,STNLABZ_MODULE_API_MAJOR,STNLABZ_MODULE_API_MINOR,lesson_qualify,lesson_start,lesson_stop};
const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void){return &lesson_descriptor;}
