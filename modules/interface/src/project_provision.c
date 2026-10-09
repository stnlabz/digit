#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"
#include "security_sa.h"
#include "account_auth.h"

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
/* [AI:GPT-6 | 2026-10-08] Directory owner must match runtime or root. */
int digit_project_directory_owner_allowed(uid_t owner, uid_t runtime_uid) {
    return owner==0 || owner==runtime_uid;
}
static int private_dir(int fd) {
    struct stat s;
    return fd>=0 && fstat(fd,&s)==0 && S_ISDIR(s.st_mode) &&
           (s.st_mode&077)==0 &&
           digit_project_directory_owner_allowed(s.st_uid,geteuid());
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
           S_ISREG(st.st_mode) && st.st_nlink==1 &&
           (st.st_mode&077)==0 &&
           (st.st_uid==0 || st.st_uid==geteuid());
}
/* [AI:GPT-6 | 2026-10-08] Revalidate an opened protected record
 * against its directory entry after reading. A replacement, relink,
 * permission change or observable in-place mutation fails closed. */
static int stable_private_record(int dir,const char *name,int fd,
                                 const struct stat *before) {
    struct stat opened,named;
    if(fstat(fd,&opened)!=0 ||
       fstatat(dir,name,&named,AT_SYMLINK_NOFOLLOW)!=0)return 0;
    if(!S_ISREG(opened.st_mode)||!S_ISREG(named.st_mode)||
       opened.st_nlink!=1||named.st_nlink!=1 ||
       (opened.st_mode&077)!=0||(named.st_mode&077)!=0 ||
       (opened.st_uid!=0&&opened.st_uid!=geteuid()) ||
       (named.st_uid!=0&&named.st_uid!=geteuid()))return 0;
    if(opened.st_dev!=before->st_dev||opened.st_ino!=before->st_ino||
       named.st_dev!=before->st_dev||named.st_ino!=before->st_ino||
       opened.st_size!=before->st_size||named.st_size!=before->st_size||
       opened.st_mtim.tv_sec!=before->st_mtim.tv_sec||
       opened.st_mtim.tv_nsec!=before->st_mtim.tv_nsec||
       opened.st_ctim.tv_sec!=before->st_ctim.tv_sec||
       opened.st_ctim.tv_nsec!=before->st_ctim.tv_nsec||
       named.st_mtim.tv_sec!=before->st_mtim.tv_sec||
       named.st_mtim.tv_nsec!=before->st_mtim.tv_nsec||
       named.st_ctim.tv_sec!=before->st_ctim.tv_sec||
       named.st_ctim.tv_nsec!=before->st_ctim.tv_nsec)return 0;
    return 1;
}
/* [AI:GPT-6 | 2026-10-08] Project identity is authoritative data,
 * not merely a protected file's existence. Parse exactly one bounded row
 * and require the claimed scope to match the directory being opened. */
static int project_metadata_matches(int dir,const char *org,const char *project) {
    int fd=openat(dir,"project.tsv",O_RDONLY|O_NOFOLLOW);
    char row[3*DIGIT_PROJECT_ID_MAX+4],*first,*second,*admin;
    ssize_t n;
    struct stat st;
    int ok=0,checked=0;
    if(fd<0)return 0;
    if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_nlink!=1 ||
       (st.st_mode&077)!=0 || (st.st_uid!=0 && st.st_uid!=geteuid()))
        goto finish;
    checked=1;
    n=read(fd,row,sizeof(row));
    /* [AI:GPT-6 | 2026-10-08] Reject hidden suffixes: C string
     * comparisons must not accept embedded NUL bytes or extra records. */
    if(n<5 || n>=(ssize_t)sizeof(row) || row[n-1]!='\n' ||
       memchr(row,'\0',(size_t)n)!=NULL ||
       memchr(row,'\n',(size_t)n-1)!=NULL ||
       st.st_size!=n)goto finish;
    row[n-1]='\0';
    first=strchr(row,'\t');
    if(!first)goto finish;
    *first++='\0';
    second=strchr(first,'\t');
    if(!second)goto finish;
    *second++='\0';
    admin=second;
    if(strchr(admin,'\t') || !project_name(admin))goto finish;
    ok=strcmp(row,org)==0 && strcmp(first,project)==0;
finish:
    if(!checked||!stable_private_record(dir,"project.tsv",fd,&st))ok=0;
    if(close(fd)!=0)ok=0;
    return ok;
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
       !private_regular(p,"project.tsv")||
       !project_metadata_matches(p,org,project))goto end;
    markerfd=openat(p,"READY",O_RDONLY|O_NOFOLLOW);
    if(markerfd<0)goto end;
    {
        struct stat before;
        if(fstat(markerfd,&before)!=0 || !S_ISREG(before.st_mode) ||
           before.st_nlink!=1 || (before.st_mode&077)!=0 ||
           (before.st_uid!=0&&before.st_uid!=geteuid()))goto end;
        n=read(markerfd,marker,sizeof(marker));
        good=n==(ssize_t)(sizeof(expected)-1) &&
             memcmp(marker,expected,sizeof(expected)-1)==0 &&
             before.st_size==n &&
             stable_private_record(p,"READY",markerfd,&before);
    }
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
    struct stat st,after,path_after;
    char *line=NULL;
    size_t line_cap=0;
    ssize_t line_size;
    if(!project_name(org)||!project_name(project)||!project_name(identity)||
       !digit_project_security_ready(root,org,project))return 0;
    base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    if(!private_dir(base))goto done;
    o=open_private(base,org);if(o<0)goto done;
    p=open_private(o,project);if(p<0)goto done;
    fd=openat(p,"security.tsv",O_RDONLY|O_NOFOLLOW);
    if(fd<0||fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||
       st.st_nlink!=1||(st.st_mode&077)!=0||
       (st.st_uid!=0&&st.st_uid!=geteuid())||
       st.st_size<=0||st.st_size>8*1024*1024)goto done;
    file=fdopen(fd,"r");
    if(!file)goto done;
    fd=-1;
    /* [AI:GPT-6 | 2026-10-08] Length-aware roster parsing rejects
     * embedded NULs, overlong records and truncated lines. */
    while((line_size=getline(&line,&line_cap,file))!=-1){
        char *tab1,*tab2;
        size_t n=(size_t)line_size;
        if(n<2 || n>=256 || line[n-1]!='\n' ||
           memchr(line,'\0',n)!=NULL){invalid=1;break;}
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
    /* [AI:GPT-6 | 2026-10-08] A roster grant is accepted only if the
     * opened inode and its project path retain protected metadata throughout
     * parsing. Detect in-place mutations and pathname replacements. */
    if(ferror(file) || fstat(fileno(file),&after)!=0 ||
       fstatat(p,"security.tsv",&path_after,AT_SYMLINK_NOFOLLOW)!=0 ||
       !S_ISREG(after.st_mode) || !S_ISREG(path_after.st_mode) ||
       after.st_nlink!=1 || path_after.st_nlink!=1 ||
       (after.st_mode&077)!=0 || (path_after.st_mode&077)!=0 ||
       (after.st_uid!=0 && after.st_uid!=geteuid()) ||
       (path_after.st_uid!=0 && path_after.st_uid!=geteuid()) ||
       after.st_dev!=st.st_dev || after.st_ino!=st.st_ino ||
       path_after.st_dev!=st.st_dev || path_after.st_ino!=st.st_ino ||
       after.st_size!=st.st_size || path_after.st_size!=st.st_size ||
       after.st_mtim.tv_sec!=st.st_mtim.tv_sec ||
       after.st_mtim.tv_nsec!=st.st_mtim.tv_nsec ||
       after.st_ctim.tv_sec!=st.st_ctim.tv_sec ||
       after.st_ctim.tv_nsec!=st.st_ctim.tv_nsec ||
       path_after.st_mtim.tv_sec!=st.st_mtim.tv_sec ||
       path_after.st_mtim.tv_nsec!=st.st_mtim.tv_nsec ||
       path_after.st_ctim.tv_sec!=st.st_ctim.tv_sec ||
       path_after.st_ctim.tv_nsec!=st.st_ctim.tv_nsec)invalid=1;
done:
    free(line);
    if(file && fclose(file)!=0)invalid=1;
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

/* [AI:GPT-6 | 2026-10-09] An SA-only directory of verified
 * restricted project members. Does not equate account, membership and SA.
 * Malformed, duplicate or mutable roster data fails closed. */
int digit_project_members_json(const char *root,const char *org,
 const char *project,const char *actor,const char *sa_registry,
 char *json,size_t capacity){
 int base=-1,o=-1,p=-1,fd=-1,ok=0;
 FILE *file=NULL;struct stat before,after,named;
 char row[256],seen[128][DIGIT_PROJECT_USER_MAX];
 size_t count=0,used=0;
 if(!json||capacity<128||!project_name(org)||!project_name(project)||
    !project_name(actor)||!digit_security_sa_verify(sa_registry,org,actor)||
    !digit_project_security_member(root,org,project,actor))return 0;
 json[0]=0;
 base=open(root,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
 if(!private_dir(base))goto done;
 o=open_private(base,org);if(o<0)goto done;
 p=open_private(o,project);if(p<0)goto done;
 fd=openat(p,"security.tsv",O_RDONLY|O_NOFOLLOW);
 if(fd<0||fstat(fd,&before)!=0||!S_ISREG(before.st_mode)||
    before.st_nlink!=1||(before.st_mode&077)!=0||
    (before.st_uid!=0&&before.st_uid!=geteuid())||
    before.st_size<=0||before.st_size>8*1024*1024)goto done;
 file=fdopen(fd,"r");if(!file)goto done;fd=-1;
 {int n=snprintf(json,capacity,
   "{\"organization\":\"%s\",\"project\":\"%s\",\"members\":[",org,project);
  if(n<0||(size_t)n>=capacity)goto done;used=(size_t)n;}
 while(fgets(row,sizeof(row),file)){
  char *first,*second,*user;size_t i,n=strlen(row);
  int length;
  if(n<2||row[n-1]!='\n'||memchr(row,'\0',n-1)!=NULL||
     count>=128)goto done;
  row[n-1]=0;first=row;second=strchr(first,'\t');
  if(!second)goto done;*second++=0;user=strchr(second,'\t');
  if(!user)goto done;*user++=0;
  if(strchr(user,'\t')||strcmp(first,"security")||
     strcmp(second,"restricted")||!project_name(user)||
     strlen(user)>=DIGIT_PROJECT_USER_MAX)goto done;
  for(i=0;i<count;i++)if(!strcmp(seen[i],user))goto done;
  strcpy(seen[count],user);
  length=snprintf(json+used,capacity-used,
     "%s{\"user\":\"%s\",\"membership\":\"restricted\",\"account_verified_active\":%s,\"sa_authorized\":%s}",
     count?",":"",user,
     digit_account_active_file(DIGIT_ACCOUNT_AUTH_PATH,user)?"true":"false",
     digit_security_sa_verify(sa_registry,org,user)?"true":"false");
  if(length<0||(size_t)length>=capacity-used)goto done;
  used+=(size_t)length;count++;
 }
 if(ferror(file)||fstat(fileno(file),&after)!=0||
    fstatat(p,"security.tsv",&named,AT_SYMLINK_NOFOLLOW)!=0||
    !S_ISREG(after.st_mode)||!S_ISREG(named.st_mode)||
    after.st_nlink!=1||named.st_nlink!=1||
    (after.st_mode&077)||(named.st_mode&077)||
    after.st_dev!=before.st_dev||after.st_ino!=before.st_ino||
    named.st_dev!=before.st_dev||named.st_ino!=before.st_ino||
    after.st_size!=before.st_size||named.st_size!=before.st_size||
    after.st_mtim.tv_sec!=before.st_mtim.tv_sec||
    after.st_mtim.tv_nsec!=before.st_mtim.tv_nsec||
    named.st_mtim.tv_sec!=before.st_mtim.tv_sec||
    named.st_mtim.tv_nsec!=before.st_mtim.tv_nsec||
    after.st_ctim.tv_sec!=before.st_ctim.tv_sec||
    after.st_ctim.tv_nsec!=before.st_ctim.tv_nsec||
    named.st_ctim.tv_sec!=before.st_ctim.tv_sec||
    named.st_ctim.tv_nsec!=before.st_ctim.tv_nsec)goto done;
 if(used+4>=capacity)goto done;
 memcpy(json+used,"]}\n",4);
 ok=1;
done:
 if(!ok&&json&&capacity)json[0]=0;
 if(file)fclose(file);if(fd>=0)close(fd);
 if(p>=0)close(p);if(o>=0)close(o);if(base>=0)close(base);
 return ok;
}
