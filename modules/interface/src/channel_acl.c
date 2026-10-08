#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "channel_acl.h"
#include "project_channel_bridge.h"
#include "project_provision.h"
#include "security_sa.h"

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
int digit_channel_acl_check_scoped(const char *path,const char *project_root,
                                  const char *sa_registry,const char *verified_user,
                                  const char *channel_id)
{
    FILE *file;
    int fd;
    struct stat st;
    char line[1024];
    int matched=0,ok=0,malformed=0;
    if(!path || !acl_id(verified_user) || !acl_id(channel_id))return 0;
    /* [AI:GPT-6 | 2026-10-08] The grant record itself must be a private
     * regular file. Reject symlinks before opening it for parsing. */
    fd=open(path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
    if(fd<0)return 0;
    if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) ||
       (st.st_mode & 077)!=0 || (st.st_uid!=0 && st.st_uid!=geteuid())) {
        close(fd);return 0;
    }
    /* [AI:GPT-6 | 2026-10-08] Bound the protected ACL snapshot before
     * parsing. Reject oversized or empty authorization registries rather
     * than consuming unbounded input. The 8 MiB limit applies to files. */
    if(st.st_size<=0 || st.st_size>8*1024*1024){
        close(fd);return 0;
    }
    file=fdopen(fd,"r");
    if(!file){close(fd);return 0;}
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
            if(ok) {
                char owner_org[DIGIT_PROJECT_ID_MAX],owner_project[DIGIT_PROJECT_ID_MAX];
                int ownership=digit_project_security_owner(project_root,channel_id,
                    owner_org,sizeof(owner_org),owner_project,sizeof(owner_project));
                if(ownership<0 || (ownership==1 &&
                   (strcmp(owner_org,fields[1])!=0||
                    strcmp(owner_project,fields[2])!=0)))ok=0;
            }
            if(ok && digit_project_security_ready(project_root,fields[1],fields[2])) {
                char security_id[DIGIT_CHANNEL_ID_MAX];
                /* [AI:GPT-6 | 2026-10-08] No project channel is
                 * accessible until its restricted security Core binding
                 * is complete. For #security, SA is mandatory, regardless
                 * of any mistaken general ACL entry.
                 */
                if(!digit_project_security_channel_id(project_root,fields[1],
                     fields[2],security_id,sizeof(security_id)))
                    ok=0;
                else if(strcmp(security_id,channel_id)==0 &&
                        (!digit_security_sa_verify(sa_registry,fields[1],verified_user) ||
                         !digit_project_security_member(project_root,fields[1],fields[2],verified_user)))
                    ok=0;
            }
        }
    }
    if(ferror(file))malformed=1;
    fclose(file);
    return !malformed && matched==1 && ok;
}

int digit_channel_acl_check_file(const char *path,const char *verified_user,
                                  const char *channel_id)
{
    return digit_channel_acl_check_scoped(path,DIGIT_PROJECT_ROOT,
           DIGIT_SECURITY_SA_REGISTRY,verified_user,channel_id);
}
