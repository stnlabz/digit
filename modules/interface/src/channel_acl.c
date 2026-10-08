#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "channel_acl.h"

/* [AI:GPT-6 | 2026-10-08] Protected, bounded ACL snapshot.
 * Every record must parse correctly; conflicting duplicate scopes deny.
 * No implicit default grant. Operator provisioned, never conversation edited.
 */
static int acl_id(const char *s)
{
    size_t i,n;
    if(!s)return 0;
    n=strnlen(s,128);
    if(n==0 || n>=128)return 0;
    for(i=0;i<n;++i)
        if(!((s[i]>='a'&&s[i]<='z') || (s[i]>='A'&&s[i]<='Z') ||
             (s[i]>='0'&&s[i]<='9') || s[i]=='-' || s[i]=='_' || s[i]=='.'))
            return 0;
    return 1;
}
static int acl_flag(const char *s)
{
    return strcmp(s,"0")==0 || strcmp(s,"1")==0;
}
int digit_channel_acl_check_file(const char *path, const char *verified_user,
                                  const char *channel_id)
{
    FILE *file;
    struct stat st;
    char line[1024];
    int matched=0,ok=0,malformed=0;
    if(!path || !acl_id(verified_user) || !acl_id(channel_id))return 0;
    file=fopen(path,"r");
    if(!file)return 0;
    if(fstat(fileno(file),&st)!=0 || !S_ISREG(st.st_mode) ||
       (st.st_mode & 077)!=0 || (st.st_uid!=0 && st.st_uid!=geteuid())) {
        fclose(file);return 0;
    }
    while(fgets(line,sizeof(line),file)) {
        char *fields[8],*p=line;
        size_t i,n=strlen(line);
        if(n==0 || line[n-1]!='\n'){malformed=1;break;}
        line[n-1]='\0';
        for(i=0;i<8;++i) {
            char *tab;
            fields[i]=p;
            if(i==7) break;
            tab=strchr(p,'\t');
            if(!tab)break;
            *tab='\0';p=tab+1;
        }
        if(i!=7 || strchr(fields[7],'\t') ||
           !acl_id(fields[0]) || !acl_id(fields[1]) ||
           !acl_id(fields[2]) || !acl_id(fields[3]) ||
           !acl_flag(fields[4]) || !acl_flag(fields[5]) ||
           !acl_flag(fields[6]) || !acl_flag(fields[7])) {
            malformed=1;break;
        }
        if(strcmp(fields[0],verified_user)!=0 ||
           strcmp(fields[3],channel_id)!=0)continue;
        ++matched;
        if(matched>1){malformed=1;break;}
        {
            digit_access_principal_t principal={
                verified_user,fields[1],1,1,
                strcmp(fields[4],"1")==0,
                strcmp(fields[5],"1")==0,
                strcmp(fields[6],"1")==0
            };
            digit_access_resource_t resource={
                fields[1],fields[2],fields[3],verified_user,
                strcmp(fields[7],"1")==0
            };
            ok=digit_access_evaluate(&principal,&resource)==DIGIT_ACCESS_ELIGIBLE;
        }
    }
    if(ferror(file))malformed=1;
    fclose(file);
    return !malformed && matched==1 && ok;
}
