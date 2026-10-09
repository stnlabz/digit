#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_provision.h"

/* [AI:GPT-6 | 2026-10-08] Project bootstrap negative tests. */
static unsigned int total=0,failures=0;
static void check(int ok,const char *name) {
    ++total;
    if(ok)printf("PASS %02u - %s\n",total,name);
    else {++failures;printf("FAIL %02u - %s\n",total,name);}
}
int main(void) {
    char root[]="/tmp/digit-project-XXXXXX";
    char project[256],security[300],ready[300],org[256],registry[256];
    FILE *f;
    char line[120];
    if(!mkdtemp(root))return 1;
    snprintf(project,sizeof(project),"%s/stn-labz/digit",root);
    snprintf(org,sizeof(org),"%s/stn-labz",root);
    snprintf(security,sizeof(security),"%s/security.tsv",project);
    snprintf(ready,sizeof(ready),"%s/READY",project);
    snprintf(registry,sizeof(registry),"%s/sa.tsv",root);
    f=fopen(registry,"w");if(!f)return 1;
    fputs("poe\tstn-labz\tADMIN\t1\t1\t1\n",f);
    fputs("sysadmin\tstn-labz\tSA\t1\t1\t1\n",f);
    fclose(f);chmod(registry,0600);
    check(!digit_project_security_ready(root,"stn-labz","digit"),"absent project not ready");
    check(!digit_project_provision(root,"stn-labz","digit","digit","sysadmin",registry),"Digit cannot self-approve founding administrator");
    check(!digit_project_provision(root,"bad/name","digit","poe","sysadmin",registry),"invalid organization denied");
    check(!digit_project_provision(root,"stn-labz","../bad","poe","sysadmin",registry),"path traversal denied");
    check(!digit_project_provision(root,"stn-labz","digit","bad/user","sysadmin",registry),"invalid founder denied");
    check(!digit_project_provision(root,"stn-labz","digit","poe","poe",registry),"ordinary admin cannot serve as SA");
    check(digit_project_provision(root,"stn-labz","digit","poe","sysadmin",registry),"project provision succeeds");
    check(digit_project_security_ready(root,"stn-labz","digit"),"restricted security record published");
    {
        char alias[320];
        snprintf(alias,sizeof(alias),"%s/READY-alias",project);
        check(link(ready,alias)==0,"READY hard-link fixture created");
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "hard-linked READY marker denied");
        check(unlink(alias)==0,"READY alias removed");
        check(digit_project_security_ready(root,"stn-labz","digit"),
              "single-link READY marker restored");
        snprintf(alias,sizeof(alias),"%s/security-alias",project);
        check(link(security,alias)==0,"security membership hard-link fixture created");
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "hard-linked security membership record denied");
        check(!digit_project_security_member(root,"stn-labz","digit","sysadmin"),
              "hard-linked membership cannot authorize SA");
        check(unlink(alias)==0,"membership alias removed");
        check(digit_project_security_ready(root,"stn-labz","digit") &&
              digit_project_security_member(root,"stn-labz","digit","sysadmin"),
              "single-link security record restores membership");
    }

    /* [AI:GPT-6 | 2026-10-08] A protected file with the wrong project
     * identity must not count as a ready project. */
    {
        char metadata[320];
        snprintf(metadata,sizeof(metadata),"%s/project.tsv",project);
        f=fopen(metadata,"w");if(!f)return 1;
        fputs("stn-labz\tother\tpoe\n",f);
        if(fclose(f)!=0)return 1;
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "mismatched project identity denied");
        f=fopen(metadata,"w");if(!f)return 1;
        fputs("stn-labz\tdigit\tbad/user\n",f);
        if(fclose(f)!=0)return 1;
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "malformed project administrator denied");
        f=fopen(metadata,"w");if(!f)return 1;
        fputs("stn-labz\tdigit\tpoe\nunexpected\n",f);
        if(fclose(f)!=0)return 1;
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "multiple project metadata rows denied");
        {
            static const char nul_row[]="stn-labz\tdigit\tpoe\0hidden\n";
            f=fopen(metadata,"wb");if(!f)return 1;
            if(fwrite(nul_row,1,sizeof(nul_row)-1,f)!=sizeof(nul_row)-1 ||
               fclose(f)!=0)return 1;
            check(!digit_project_security_ready(root,"stn-labz","digit"),
                  "embedded NUL project metadata denied");
        }
        f=fopen(metadata,"w");if(!f)return 1;
        fputs("stn-labz\tdigit\tpoe\nextra\n",f);
        if(fclose(f)!=0)return 1;
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "additional complete metadata record denied");
        f=fopen(metadata,"w");if(!f)return 1;
        fputs("stn-labz\tdigit\tpoe\n",f);
        if(fclose(f)!=0)return 1;
        check(digit_project_security_ready(root,"stn-labz","digit"),
              "restored matching project identity accepted");
    }
    /* [AI:GPT-6 | 2026-10-08] A valid SA must not be authorized
     * through a binary or overlong security roster record. */
    {
        static const char nul_member[]=
            "security\trestricted\tdigit\n"
            "security\trestricted\tsysadmin\0hidden\n";
        f=fopen(security,"wb");if(!f)return 1;
        if(fwrite(nul_member,1,sizeof(nul_member)-1,f)!=sizeof(nul_member)-1)
            return 1;
        if(fclose(f)!=0)return 1;
        check(!digit_project_security_member(root,"stn-labz","digit","sysadmin"),
              "embedded NUL security membership denied");
        f=fopen(security,"w");if(!f)return 1;
        fputs("security\trestricted\tdigit\n"
              "security\trestricted\tsysadmin",f);
        if(fclose(f)!=0)return 1;
        check(!digit_project_security_member(root,"stn-labz","digit","sysadmin"),
              "unterminated security roster record denied");
        f=fopen(security,"w");if(!f)return 1;
        fputs("security\trestricted\tdigit\n"
              "security\trestricted\tsysadmin\n",f);
        if(fclose(f)!=0)return 1;
        check(digit_project_security_member(root,"stn-labz","digit","sysadmin"),
              "restored valid security roster accepted");
    }
    /* [AI:GPT-6 | 2026-10-08] Protected membership paths must not
     * redirect through an alias, even when the contents remain valid. */
    {
        char backup[320];
        snprintf(backup,sizeof(backup),"%s.security-original",security);
        check(rename(security,backup)==0,"security roster renamed for alias fixture");
        check(symlink(backup,security)==0,"security roster symlink alias created");
        check(!digit_project_security_member(root,"stn-labz","digit","sysadmin"),
              "symlinked security roster cannot authorize membership");
        check(unlink(security)==0 && rename(backup,security)==0,
              "original security roster restored from alias");
        check(digit_project_security_member(root,"stn-labz","digit","sysadmin"),
              "restored private roster authorizes qualified member");
    }
    /* [AI:GPT-6 | 2026-10-08] Project record aliases are not trusted. */
    {
        char metadata[320],alias[350];
        snprintf(metadata,sizeof(metadata),"%s/project.tsv",project);
        snprintf(alias,sizeof(alias),"%s/project-alias",project);
        check(rename(metadata,alias)==0,"metadata move fixture");
        check(symlink(alias,metadata)==0,"metadata alias fixture");
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "metadata alias is denied");
        check(unlink(metadata)==0 && rename(alias,metadata)==0,
              "metadata pathname restored");
        check(digit_project_security_ready(root,"stn-labz","digit"),
              "metadata restoration accepted");
    }
    /* [AI:GPT-6 | 2026-10-08] READY path must reference its own file. */
    {
        char alias[350];
        snprintf(alias,sizeof(alias),"%s/READY-alternate",project);
        check(rename(ready,alias)==0,"READY move fixture");
        check(symlink(alias,ready)==0,"READY alias fixture");
        check(!digit_project_security_ready(root,"stn-labz","digit"),
              "READY alias is denied");
        check(unlink(ready)==0 && rename(alias,ready)==0,
              "READY pathname restored");
        check(digit_project_security_ready(root,"stn-labz","digit"),
              "READY restoration accepted");
    }
    chmod(security,0644);
    check(!digit_project_security_ready(root,"stn-labz","digit"),"readable-by-others security record denied");
    chmod(security,0600);
    check(digit_project_security_ready(root,"stn-labz","digit"),"private security record accepted");
    chmod(ready,0644);
    check(!digit_project_security_ready(root,"stn-labz","digit"),"insecure READY marker denied");
    chmod(ready,0600);
    f=fopen(ready,"w");
    if(!f)return 1;
    fputs("incorrect\n",f);
    fclose(f);
    check(!digit_project_security_ready(root,"stn-labz","digit"),"corrupt READY marker denied");
    f=fopen(ready,"w");
    if(!f)return 1;
    fputs("security-initialized\n",f);
    fclose(f);
    check(digit_project_security_ready(root,"stn-labz","digit"),"restored READY accepted");
    check(!digit_project_provision(root,"stn-labz","digit","poe","sysadmin",registry),"duplicate project denied");
    check(!digit_project_security_ready(root,"stn-labz","other"),"unregistered project denied");
    f=fopen(security,"r");
    check(f!=NULL,"security membership file exists");
    if(f) {
        int digit=0,founder=0,regular=0;
        while(fgets(line,sizeof(line),f)) {
            if(strcmp(line,"security\trestricted\tdigit\n")==0)digit=1;
            if(strcmp(line,"security\trestricted\tsysadmin\n")==0)founder=1;
            if(strcmp(line,"security\trestricted\tpoe\n")==0)regular=1;
        }
        fclose(f);
        check(digit,"Digit initially assigned to restricted security");
        check(founder,"verified SA initially assigned");
        check(!regular,"ordinary founding admin excluded from security");
    }
    check(!digit_project_provision(root,"stn-labz","other","poe","unknown",registry),"unknown SA denied");
    check(!digit_project_security_ready(root,"stn-labz","bad/name"),"unsafe channel path denied");
    check(!digit_project_security_ready(root,"elsewhere","digit"),"organization isolation");
    check(!digit_project_provision(NULL,"stn-labz","other","poe","sysadmin",registry),"null root denied");
    printf("\nProject provisioning tests: %u executed, %u failed\n",total,failures);
    return failures!=0;
}
