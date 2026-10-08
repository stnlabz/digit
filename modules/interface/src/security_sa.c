#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "security_sa.h"

/* [AI:GPT-6 | 2026-10-08] Fail closed on malformed, duplicate,
 * unprotected, or unauthorized SA records. General administrators are never
 * implicitly SAs. Read on each decision so deassignment takes effect.
 */
static int sa_ident(const char *s) {
    size_t i,n;
    if(!s)return 0;
    n=strnlen(s,64);
    if(!n||n>=64)return 0;
    for(i=0;i<n;++i)
        if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||
             (s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='_'||s[i]=='.'))return 0;
    return 1;
}
int digit_security_sa_verify(const char *registry,const char *organization,
                             const char *identity) {
    FILE *f;
    struct stat st;
    char line[512];
    int found=0,authorized=0,invalid=0;
    if(!registry||!sa_ident(organization)||!sa_ident(identity)||
       strcmp(identity,"digit")==0)return 0;
    f=fopen(registry,"r");
    if(!f)return 0;
    if(fstat(fileno(f),&st)!=0||!S_ISREG(st.st_mode)||
       (st.st_mode&077)!=0||(st.st_uid!=0 && st.st_uid!=geteuid())) {
        fclose(f);return 0;
    }
    while(fgets(line,sizeof(line),f)) {
        char *field[6],*p=line,*tab;
        size_t i,n=strlen(line);
        if(n==0||line[n-1]!='\n'){invalid=1;break;}
        line[n-1]='\0';
        for(i=0;i<6;++i) {
            field[i]=p;
            if(i==5)break;
            tab=strchr(p,'\t');
            if(!tab)break;
            *tab='\0';p=tab+1;
        }
        if(i!=5||strchr(field[5],'\t')||
           !sa_ident(field[0])||!sa_ident(field[1])||
           (strcmp(field[2],"SA")!=0 && strcmp(field[2],"ADMIN")!=0)||
           (strcmp(field[3],"0")!=0&&strcmp(field[3],"1")!=0)||
           (strcmp(field[4],"0")!=0&&strcmp(field[4],"1")!=0)||
           (strcmp(field[5],"0")!=0&&strcmp(field[5],"1")!=0)){
            invalid=1;break;
        }
        if(strcmp(field[0],identity)!=0 || strcmp(field[1],organization)!=0)continue;
        if(++found>1){invalid=1;break;}
        authorized=strcmp(field[2],"SA")==0 &&
                   strcmp(field[3],"1")==0 &&
                   strcmp(field[4],"1")==0 &&
                   strcmp(field[5],"1")==0;
    }
    if(ferror(f))invalid=1;
    fclose(f);
    return !invalid && found==1 && authorized;
}
