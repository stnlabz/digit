#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "alert.h"
#include "channel.h"

#define DIGIT_ALERT_STATE_DIR "/opt/digit/state/alerts"
#define DIGIT_ALERTS_FILE DIGIT_ALERT_STATE_DIR "/alerts.tsv"
#define DIGIT_ALERT_ACK_FILE DIGIT_ALERT_STATE_DIR "/ack.tsv"

static unsigned long long digit_alert_sequence = 0;

static unsigned long long digit_alert_now(void){return (unsigned long long)time(NULL);}
static int digit_alert_safe(const char *text){return text!=NULL&&strchr(text,'\t')==NULL&&strchr(text,'\n')==NULL&&strchr(text,'\r')==NULL;}
static int digit_alert_dir(void){return mkdir(DIGIT_ALERT_STATE_DIR,0750)==0||errno==EEXIST;}
static void digit_alert_id(char *out,size_t size){++digit_alert_sequence;(void)snprintf(out,size,"alert-%llu-%llu",digit_alert_now(),digit_alert_sequence);}

const char *digit_alert_severity_string(digit_alert_severity_t severity)
{
    switch(severity){case DIGIT_ALERT_INFO:return "INFO";case DIGIT_ALERT_WARNING:return "WARNING";case DIGIT_ALERT_ERROR:return "ERROR";case DIGIT_ALERT_CRITICAL:return "CRITICAL";default:return "UNKNOWN";}
}

int digit_alert_init(void){return digit_alert_dir();}

static int digit_alert_is_acknowledged(const char *id,unsigned long long *when)
{
    FILE *file;char stored[DIGIT_ALERT_ID_MAX];unsigned long long timestamp;
    file=fopen(DIGIT_ALERT_ACK_FILE,"r");if(file==NULL)return 0;
    while(fscanf(file,"%63s\t%llu\n",stored,&timestamp)==2)
    {if(strcmp(stored,id)==0){if(when!=NULL)*when=timestamp;fclose(file);return 1;}}
    fclose(file);return 0;
}

static int digit_alert_find_channel(digit_channel_t *channel)
{
    digit_channel_t channels[DIGIT_CHANNEL_MAX];size_t count,index;
    if(channel==NULL)return 0;
    count=digit_channel_list(channels,DIGIT_CHANNEL_MAX);if(count>DIGIT_CHANNEL_MAX)count=DIGIT_CHANNEL_MAX;
    for(index=0;index<count;++index)
    {
        if(_POSIX_VERSION && strcasecmp(channels[index].name,"Alerts")==0){*channel=channels[index];return 1;}
    }
    return 0;
}

static void digit_alert_speak(const digit_alert_t *alert)
{
    digit_channel_t channel;digit_channel_message_t message;char body[DIGIT_CHANNEL_MESSAGE_MAX];int written;
    if(alert==NULL||!digit_alert_find_channel(&channel))return;
    written=snprintf(body,sizeof(body),"%s alert from %s. %s. %s Operational state: %s. Alert ID: %s.",
                     digit_alert_severity_string(alert->severity),alert->source,alert->summary,alert->detail,
                     alert->operational_state,alert->id);
    if(written<=0||(size_t)written>=sizeof(body))return;
    (void)digit_channel_message_append(channel.id,"digit",body,&message);
}

int digit_alert_raise(digit_alert_severity_t severity,const char *source,const char *summary,const char *detail,const char *operational_state,digit_alert_t *alert)
{
    FILE *file;digit_alert_t created;
    if(severity<DIGIT_ALERT_INFO||severity>DIGIT_ALERT_CRITICAL||!digit_alert_safe(source)||!digit_alert_safe(summary)||!digit_alert_safe(detail)||!digit_alert_safe(operational_state))return 0;
    if(strlen(source)>=sizeof(created.source)||strlen(summary)>=sizeof(created.summary)||strlen(detail)>=sizeof(created.detail)||strlen(operational_state)>=sizeof(created.operational_state))return 0;
    if(!digit_alert_dir())return 0;
    memset(&created,0,sizeof(created));digit_alert_id(created.id,sizeof(created.id));created.created_at=digit_alert_now();created.severity=severity;
    (void)snprintf(created.source,sizeof(created.source),"%s",source);(void)snprintf(created.summary,sizeof(created.summary),"%s",summary);(void)snprintf(created.detail,sizeof(created.detail),"%s",detail);(void)snprintf(created.operational_state,sizeof(created.operational_state),"%s",operational_state);
    file=fopen(DIGIT_ALERTS_FILE,"a");if(file==NULL)return 0;
    if(fprintf(file,"%s\t%llu\t%d\t%s\t%s\t%s\t%s\n",created.id,created.created_at,(int)created.severity,created.source,created.summary,created.detail,created.operational_state)<0){fclose(file);return 0;}
    if(fclose(file)!=0)return 0;
    digit_alert_speak(&created);
    if(alert!=NULL)*alert=created;return 1;
}

size_t digit_alert_list(digit_alert_t *alerts,size_t capacity,int unacknowledged_only)
{
    FILE *file;char line[1600];size_t count=0;
    file=fopen(DIGIT_ALERTS_FILE,"r");if(file==NULL)return 0;
    while(fgets(line,sizeof(line),file)!=NULL)
    {
        digit_alert_t item;int severity;memset(&item,0,sizeof(item));
        if(sscanf(line,"%63[^\t]\t%llu\t%d\t%63[^\t]\t%255[^\t]\t%1023[^\t]\t%31[^\n]",item.id,&item.created_at,&severity,item.source,item.summary,item.detail,item.operational_state)!=7)continue;
        item.severity=(digit_alert_severity_t)severity;item.acknowledged=digit_alert_is_acknowledged(item.id,&item.acknowledged_at);
        if(unacknowledged_only&&item.acknowledged)continue;
        if(alerts!=NULL&&count<capacity)alerts[count]=item;++count;
    }
    fclose(file);return count;
}

int digit_alert_get(const char *alert_id,digit_alert_t *alert)
{
    digit_alert_t items[DIGIT_ALERT_MAX];size_t count,index;if(alert_id==NULL||alert==NULL)return 0;
    count=digit_alert_list(items,DIGIT_ALERT_MAX,0);if(count>DIGIT_ALERT_MAX)count=DIGIT_ALERT_MAX;
    for(index=0;index<count;++index)if(strcmp(items[index].id,alert_id)==0){*alert=items[index];return 1;}return 0;
}

int digit_alert_acknowledge(const char *alert_id,digit_alert_t *alert)
{
    FILE *file;digit_alert_t found;unsigned long long now;
    if(!digit_alert_get(alert_id,&found))return 0;if(found.acknowledged){if(alert!=NULL)*alert=found;return 1;}
    if(!digit_alert_dir())return 0;file=fopen(DIGIT_ALERT_ACK_FILE,"a");if(file==NULL)return 0;now=digit_alert_now();
    if(fprintf(file,"%s\t%llu\n",alert_id,now)<0){fclose(file);return 0;}if(fclose(file)!=0)return 0;
    found.acknowledged=1;found.acknowledged_at=now;if(alert!=NULL)*alert=found;return 1;
}
