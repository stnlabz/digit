#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "project_list.h"
static unsigned done,failed;
static void check(int pass,const char *name){++done;
 if(pass)printf("PASS %02u - %s\n",done,name);
 else{++failed;printf("FAIL %02u - %s\n",done,name);}}
int main(void){
 char root[]="/tmp/digit-project-list-XXXXXX",out[512],p[256];
 check(mkdtemp(root)!=NULL,"private fixture root created");
 check(chmod(root,0700)==0,"root protected");
 check(digit_project_list_scoped(root,"stn-labz","poemei",out,sizeof(out)),"missing org empty listing");
 check(strstr(out,"\"projects\":[]")!=NULL,"empty listing serialized");
 check(!digit_project_list_scoped(root,"../other","poemei",out,sizeof(out)),"traversal rejected");
 check(!digit_project_list_scoped(root,"","poemei",out,sizeof(out)),"empty organization rejected");
 check(!digit_project_list_scoped(root,NULL,"poemei",out,sizeof(out)),"null organization rejected");
 check(!digit_project_list_scoped(NULL,"stn-labz","poemei",out,sizeof(out)),"null root rejected");
 check(!digit_project_list_scoped(root,"stn-labz","poemei",NULL,sizeof(out)),"null output rejected");
 check(!digit_project_list_scoped(root,"stn-labz","poemei",out,1),"short output rejected");
 snprintf(p,sizeof(p),"%s/stn-labz",root);check(mkdir(p,0700)==0,"org directory created");
 check(digit_project_list_scoped(root,"stn-labz","poemei",out,sizeof(out)),"empty org listed");
 check(strstr(out,"\"organization\":\"stn-labz\"")!=NULL,"exact org in response");
 check(!digit_project_list_scoped(root,"stn.labz","poemei",out,sizeof(out)),"invalid organization denied");
 check(chmod(p,0755)==0,"unsafe org fixture");
 check(!digit_project_list_scoped(root,"stn-labz","poemei",out,sizeof(out)),"unsafe org permissions denied");
 chmod(p,0700);
 rmdir(p);rmdir(root);
 printf("Interface 1.5.10 milestone: %u executed, %u failed\n",done,failed);
 return failed?1:0;
}
