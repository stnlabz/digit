#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"
#include "security_sa.h"

/* [AI:GPT-6 | 2026-10-08] READY is published only after both records.
 * Incomplete folders are NOT provisioned projects.
 */
static int project_name(const char *name) {
    size_t i,n;
    if(!name)return 0;
    n=strnlen(name,DIGIT_PROJECT_ID_MAX);
    if(!n||n>=DIGIT_PROJECT_ID_MAX)return 0;
    for(i=0;i<n;i++)
        if(!((name[i]>='a'&&name[i]<='z')||
             (name[i]>='A'&&name[i]<='Z')||
             (name[i]>='0'&&name[i]<='9')||
             name[i]=='-'||name[i]=='_'))return 0;
    return 1;
}
static int private_dir(int fd) {
    struct stat s;
    return fd>=0 && fstat(fd,&s)==0 && S_ISDIR(s.st_mode) &&
           (s.st_mode&077)==0;
}
static int open_private(int dir,const char *name) {
    int fd=openat(dir,name,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(fd)){if(fd>=0)close(fd);return -1;}
    return fd;
}
static int record(int dir,const char *name,const char *value) {
    int fd=openat(dir,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
    size_t n=strlen(value);
    int good=0;
    if(fd<0)return 0;
    if(write(fd,value,n)==(ssize_t)n && fsync(fd)==0)good=1;
    if(close(fd)!=0)good=0;
    return good;
}
/* [AI:GPT-6 | 2026-10-08] READY alone is not sufficient:
 * both private records must be regular files belonging to the runtime.
 * The expected completion marker must be intact.
 */
static int private_regular(int dir,const char *name) {
    struct stat st;
    return fstatat(dir,name,&st,AT_SYMLINK_NOFOLLOW)==0 &&
           S_ISREG(st.st_mode) && (st.st_mode&077)==0 &&
           (st.st_uid==0 || st.st_uid==geteuid());
}
int digit_project_security_ready(const char *root,const char *org,const char *project) {
    int base=-1,o=-1,p=-1,markerfd=-1,good=0;
    char marker[32];
    ssize_t n;
    static const char expected[]="security-initialized\n";
    if(!root||!project_name(org)||!project_name(project))return 0;
    base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(base))goto end;
    o=open_private(base,org);if(o<0)goto end;
    p=open_private(o,project);if(p<0)goto end;
    if(!private_regular(p,"READY")||!private_regular(p,"security.tsv")||
       !private_regular(p,"project.tsv"))goto end;
    markerfd=openat(p,"READY",O_RDONLY|O_NOFOLLOW);
    if(markerfd<0)goto end;
    n=read(markerfd,marker,sizeof(marker));
    good=n==(ssize_t)(sizeof(expected)-1) &&
         memcmp(marker,expected,sizeof(expected)-1)==0;
end:
    if(markerfd>=0)close(markerfd);
    if(p>=0)close(p);
    if(o>=0)close(o);
    if(base>=0)close(base);
    return good;
}
/* [AI:GPT-6 | 2026-10-08] Membership is an authoritative private
 * project record. Fail closed on every malformed or duplicate entry.
 * A regular administrator never receives implied membership.
 */
int digit_project_security_member(const char *root,const char *org,
                                  const char *project,const char *identity)
{
    int base=-1,o=-1,p=-1,fd=-1,found=0,invalid=0;
    FILE *file=NULL;
    struct stat st;
    char line[256];
    if(!project_name(org)||!project_name(project)||!project_name(identity)||
       !digit_project_security_ready(root,org,project))return 0;
    base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(base))goto done;
    o=open_private(base,org);if(o<0)goto done;
    p=open_private(o,project);if(p<0)goto done;
    fd=openat(p,"security.tsv",O_RDONLY|O_NOFOLLOW);
    if(fd<0||fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||
       (st.st_mode&077)!=0||
       (st.st_uid!=0&&st.st_uid!=geteuid()))goto done;
    file=fdopen(fd,"r");
    if(!file)goto done;
    fd=-1;
    while(fgets(line,sizeof(line),file)){
        char *tab1,*tab2;
        size_t n=strlen(line);
        if(!n||line[n-1]!='\n'){invalid=1;break;}
        line[n-1]='\0';
        tab1=strchr(line,'\t');
        if(!tab1){invalid=1;break;}
        *tab1++='\0';
        tab2=strchr(tab1,'\t');
        if(!tab2){invalid=1;break;}
        *tab2++='\0';
        if(strchr(tab2,'\t')||strcmp(line,"security")!=0||
           strcmp(tab1,"restricted")!=0||!project_name(tab2)){
            invalid=1;break;
        }
        if(strcmp(tab2,identity)==0 && ++found>1){invalid=1;break;}
    }
    if(ferror(file))invalid=1;
done:
    if(file)fclose(file);
    if(fd>=0)close(fd);
    if(p>=0)close(p);
    if(o>=0)close(o);
    if(base>=0)close(base);
    return !invalid&&found==1;
}

int digit_project_provision(const char *root,const char *org,
                             const char *project,const char *admin,
                             const char *security_sa,const char *sa_registry) {
    int base=-1,o=-1,p=-1,good=0,created=0;
    char metadata[256],security[256];
    if(!root||!project_name(org)||!project_name(project)||
       !project_name(admin)||strcmp(admin,"digit")==0 ||
       !project_name(security_sa) ||
       !digit_security_sa_verify(sa_registry,org,security_sa))return 0;
    base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(base))goto end;
    if(mkdirat(base,org,0700)!=0 && errno!=EEXIST)goto end;
    o=open_private(base,org);if(o<0)goto end;
    if(mkdirat(o,project,0700)!=0)goto end;
    created=1;
    p=open_private(o,project);if(p<0)goto end;
    if(snprintf(metadata,sizeof(metadata),"%s\t%s\t%s\n",org,project,admin)<0)goto end;
    if(snprintf(security,sizeof(security),"security\trestricted\tdigit\nsecurity\trestricted\t%s\n",security_sa)<0)goto end;
    if(!record(p,"project.tsv",metadata)||
       !record(p,"security.tsv",security)||
       !record(p,"READY","security-initialized\n"))goto end;
    good=fsync(p)==0;
end:
    if(!good && created && p>=0){
        (void)unlinkat(p,"READY",0);
        (void)unlinkat(p,"security.tsv",0);
        (void)unlinkat(p,"project.tsv",0);
    }
    if(p>=0)close(p);
    if(!good && created && o>=0)(void)unlinkat(o,project,AT_REMOVEDIR);
    if(o>=0)close(o);
    if(base>=0)close(base);
    return good;
}
