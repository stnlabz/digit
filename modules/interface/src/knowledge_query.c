#include <stdio.h>
#include <string.h>
#include "knowledge_query.h"
/* [AI:GPT-6 | 2026-10-08] Evidence is returned as records, not
 * invented prose. Invalid or unproven sources fail closed. */
static size_t bounded(const char *s,size_t capacity)
{
    size_t n=0;
    if(!s)return capacity;
    while(n<capacity && s[n])++n;
    return n;
}
/* [AI:GPT-6 | 2026-10-08] Interface 1.4.4: reject malformed
 * UTF-8 before passing knowledge to JSON. Accept canonical Unicode scalar
 * values only: no overlong encodings, surrogate values or invalid tails. */
static int digit_knowledge_utf8_valid(const unsigned char *s,size_t n)
{
    size_t i=0;
    while(i<n){
        unsigned char a=s[i],b,c,d;
        if(a<0x80){++i;continue;}
        if(a<0xc2 || a>0xf4)return 0;
        if(a<=0xdf){
            if(i+1>=n)return 0;
            b=s[i+1];
            if(b<0x80 || b>0xbf)return 0;
            i+=2;continue;
        }
        if(a<=0xef){
            if(i+2>=n)return 0;
            b=s[i+1];c=s[i+2];
            if(b<0x80 || b>0xbf || c<0x80 || c>0xbf ||
               (a==0xe0 && b<0xa0) ||
               (a==0xed && b>=0xa0))return 0;
            i+=3;continue;
        }
        if(i+3>=n)return 0;
        b=s[i+1];c=s[i+2];d=s[i+3];
        if(b<0x80 || b>0xbf || c<0x80 || c>0xbf ||
           d<0x80 || d>0xbf ||
           (a==0xf0 && b<0x90) ||
           (a==0xf4 && b>0x8f))return 0;
        i+=4;
    }
    return 1;
}
static int printable(const char *s,size_t capacity)
{
    size_t n=bounded(s,capacity),i;
    if(!n||n>=capacity)return 0;
    for(i=0;i<n;++i){
        unsigned char ch=(unsigned char)s[i];
        if(ch<32 || ch==127)return 0;
    }
    return digit_knowledge_utf8_valid((const unsigned char *)s,n);
}
int digit_knowledge_query_valid(const char *query)
{
    return printable(query,DIGIT_KNOWLEDGE_QUERY_MAX);
}
int digit_knowledge_record_valid(const digit_knowledge_record_t *record)
{
    if(!record)return 0;
    return printable(record->id,sizeof(record->id)) &&
           printable(record->category,sizeof(record->category)) &&
           printable(record->source,sizeof(record->source)) &&
           printable(record->text,sizeof(record->text));
}
/* [AI:GPT-6 | 2026-10-08] 1.4.2: two records cannot claim
 * the same evidence identity within a single result set. Validate
 * the complete set before emitting any output. */
int digit_knowledge_results_valid(const digit_knowledge_record_t *records,
                                  size_t count)
{
    size_t i,j;
    if(count>DIGIT_KNOWLEDGE_RECORD_MAX || (count && !records))return 0;
    for(i=0;i<count;++i){
        /* [AI:GPT-6 | 2026-10-08] 1.4.5: bulk retrieval must use\n         * the same safe record-identity rule as single-record lookup. */\n        if(!digit_knowledge_record_valid(&records[i]) ||\n           !digit_knowledge_record_id_valid(records[i].id))return 0;
        for(j=0;j<i;++j){
            if(strcmp(records[i].id,records[j].id)==0)return 0;
        }
    }
    return 1;
}
static int append(char *out,size_t capacity,size_t *pos,const char *s)
{
    size_t n=strlen(s);
    if(n>=capacity-*pos)return 0;
    memcpy(out+*pos,s,n+1);
    *pos+=n;
    return 1;
}
static int escaped(char *out,size_t capacity,size_t *pos,const char *src)
{
    size_t i,n=strlen(src);
    for(i=0;i<n;++i){
        unsigned char c=(unsigned char)src[i];
        if(c=='"' || c=='\\'){
            if(capacity-*pos<=2)return 0;
            out[(*pos)++]='\\';
        }else if(capacity-*pos<=1)return 0;
        out[(*pos)++]=(char)c;
    }
    out[*pos]=0;
    return 1;
}
int digit_knowledge_result_json(const digit_knowledge_record_t *records,
                               size_t count,char *output,size_t capacity)
{
    size_t i,off=0;
    char countbuf[32];
    if(!output||!capacity)return 0;
    output[0]=0;
    if(!digit_knowledge_results_valid(records,count))return 0;
    if(!count)return append(output,capacity,&off,
        "{\"found\":false,\"answer\":\"UNKNOWN\",\"evidence_count\":0,\"records\":[]}\n");
    if(!append(output,capacity,&off,
               "{\"found\":true,\"answer\":\"EVIDENCE_ONLY\",\"evidence_count\":"))goto fail;
    (void)snprintf(countbuf,sizeof(countbuf),"%zu",count);
    if(!append(output,capacity,&off,countbuf) ||
       !append(output,capacity,&off,",\"records\":["))goto fail;
    for(i=0;i<count;++i){
        if(!digit_knowledge_record_valid(&records[i]))goto fail;
        if(!append(output,capacity,&off,i?",{\"id\":\"":"{\"id\":\"") ||
           !escaped(output,capacity,&off,records[i].id) ||
           !append(output,capacity,&off,"\",\"category\":\"") ||
           !escaped(output,capacity,&off,records[i].category) ||
           !append(output,capacity,&off,"\",\"source\":\"") ||
           !escaped(output,capacity,&off,records[i].source) ||
           !append(output,capacity,&off,"\",\"text\":\"") ||
           !escaped(output,capacity,&off,records[i].text) ||
           !append(output,capacity,&off,"\"}"))goto fail;
    }
    if(!append(output,capacity,&off,"]}\n"))goto fail;
    return 1;
fail:
    output[0]=0;
    return 0;
}

/* [AI:GPT-6 | 2026-10-08] 1.4.3: untrusted IDs never
 * enter a Core lookup or JSON response without bounded validation. */
int digit_knowledge_record_id_valid(const char *id)
{
    size_t i,n=bounded(id,65);
    if(n==0 || n>=65)return 0;
    for(i=0;i<n;++i){
        unsigned char c=(unsigned char)id[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'))return 0;
    }
    return 1;
}
int digit_knowledge_single_json(const digit_knowledge_record_t *record,
                                int found,char *output,size_t capacity)
{
    if(!output || capacity==0)return 0;
    output[0]=0;
    if(found==0)return digit_knowledge_result_json(NULL,0,output,capacity);
    if(found!=1 || !record || !digit_knowledge_record_id_valid(record->id) ||
       !digit_knowledge_record_valid(record))return 0;
    return digit_knowledge_result_json(record,1,output,capacity);
}
