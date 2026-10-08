#define _GNU_SOURCE
#include <crypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "account_auth.h"

/* [AI:GPT-6 | 2026-10-08] Fail-closed, operator-provisioned account store.
 * No passwords, hashes, or account records are logged or returned.
 * Duplicate identities and malformed records deny verification.
 */
static int account_id_valid(const char *s)
{
    size_t i,n;
    if (!s) return 0;
    n=strnlen(s,DIGIT_ACCOUNT_ID_MAX);
    if(n==0 || n>=DIGIT_ACCOUNT_ID_MAX) return 0;
    for(i=0;i<n;i++)
        if(!((s[i]>='a'&&s[i]<='z') || (s[i]>='A'&&s[i]<='Z') ||
             (s[i]>='0'&&s[i]<='9') || s[i]=='_' || s[i]=='-' || s[i]=='.')) return 0;
    return 1;
}
static int account_secure_file(FILE *f)
{
    struct stat st;
    if(fstat(fileno(f),&st)!=0 || !S_ISREG(st.st_mode)) return 0;
    if((st.st_mode & 077)!=0) return 0;
    if(st.st_uid!=0 && st.st_uid!=geteuid()) return 0;
    return 1;
}
static int account_match(const char *path,const char *user,const char *password,int verify)
{
    FILE *f;
    char line[1024],id[DIGIT_ACCOUNT_ID_MAX],state[8],hash[768];
    int matches=0,valid=0,malformed=0;
    size_t password_length=0;
    if(!path || !account_id_valid(user)) return 0;
    if(verify) {
        if(!password) return 0;
        password_length=strnlen(password,DIGIT_ACCOUNT_PASSWORD_MAX);
        if(password_length==0 || password_length>=DIGIT_ACCOUNT_PASSWORD_MAX) return 0;
    }
    f=fopen(path,"r");
    if(!f) return 0;
    if(!account_secure_file(f)){fclose(f);return 0;}
    while(fgets(line,sizeof(line),f)) {
        char *a,*b,*c,*end;
        size_t len=strlen(line);
        if(len==0 || line[len-1]!='\n') {malformed=1;break;}
        line[len-1]='\0';
        a=line;b=strchr(a,'\t');
        if(!b){malformed=1;break;}*b++='\0';
        c=strchr(b,'\t');
        if(!c){malformed=1;break;}*c++='\0';
        if(strchr(c,'\t') || !account_id_valid(a) ||
           !(strcmp(b,"0")==0 || strcmp(b,"1")==0) ||
           c[0]=='\0' || strlen(c)>=sizeof(hash)) {malformed=1;break;}
        if(strcmp(a,user)!=0) continue;
        ++matches;
        if(matches>1) {malformed=1;break;}
        if(strcmp(b,"1")!=0){valid=0;continue;}
        if(!verify) {valid=1;continue;}
        {
            struct crypt_data data;
            char *computed;
            memset(&data,0,sizeof(data));
            computed=crypt_r(password,c,&data);
            if(computed && strlen(computed)==strlen(c)) {
                size_t i;unsigned int diff=0;
                for(i=0;i<strlen(c);++i) diff|=(unsigned char)computed[i]^(unsigned char)c[i];
                valid=diff==0;
            }
            memset(&data,0,sizeof(data));
        }
        (void)end;
    }
    if(ferror(f))malformed=1;
    fclose(f);
    return !malformed && matches==1 && valid;
}
int digit_account_verify_file(const char *path,const char *user_id,const char *password)
{
    return account_match(path,user_id,password,1);
}
int digit_account_active_file(const char *path,const char *user_id)
{
    return account_match(path,user_id,NULL,0);
}
