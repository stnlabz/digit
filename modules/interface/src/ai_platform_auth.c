#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ai_platform_auth.h"
static int valid_id(const char *s){
 size_t i,n;if(!s)return 0;n=strnlen(s,64);
 if(!n||n>=64)return 0;
 for(i=0;i<n;i++){char c=s[i];if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||
 (c>='A'&&c<='Z')||c=='-'||c=='_'||c=='.'))return 0;}
 return 1;
}
int digit_ai_platform_permissions(const char *path,const char *identity,
                                  digit_ai_platform_permissions_t *out){
 int fd=-1,valid=1,matches=0;FILE *file=NULL;
 struct stat before,after;char line[512];
 digit_ai_platform_permissions_t found={0};
 if(out)memset(out,0,sizeof(*out));
 if(!out||!path||!valid_id(identity))return 0;
 fd=open(path,O_RDONLY|O_NOFOLLOW);
 if(fd<0)return 0;
 file=fdopen(fd,"r");if(!file){close(fd);return 0;}
 if(fstat(fd,&before)!=0||!S_ISREG(before.st_mode)||before.st_nlink!=1||
 (before.st_mode&077)!=0||(before.st_uid!=0&&before.st_uid!=geteuid())||
 before.st_size<0||before.st_size>1048576){valid=0;goto done;}
 while(fgets(line,sizeof(line),file)){
  char *fields[7],*p=line;size_t i,len=strlen(line);
  digit_ai_platform_permissions_t candidate={0};
  if(!len||line[len-1]!='\n'){valid=0;break;}
  line[len-1]=0;
  for(i=0;i<7;i++){
   char *tab;
   fields[i]=p;
   if(i==6)break;
   tab=strchr(p,'\t');
   if(!tab){valid=0;break;}
   *tab=0;p=tab+1;
  }
  if(i!=6||strchr(fields[6],'\t')||!valid_id(fields[0])){valid=0;break;}
  for(i=1;i<7;i++)if(strcmp(fields[i],"0")&&strcmp(fields[i],"1")){valid=0;break;}
  if(!valid)break;
  if(strcmp(fields[0],identity))continue;
  if(++matches!=1){valid=0;break;}
  candidate.active=(unsigned)(fields[1][0]=='1');
  candidate.dm=(unsigned)(fields[2][0]=='1');
  candidate.diagnostics=(unsigned)(fields[3][0]=='1');
  candidate.memory_write=(unsigned)(fields[4][0]=='1');
  candidate.learning=(unsigned)(fields[5][0]=='1');
  candidate.developer=(unsigned)(fields[6][0]=='1');
  found=candidate;
 }
 if(ferror(file))valid=0;
 if(fstat(fd,&after)!=0||after.st_dev!=before.st_dev||
 after.st_ino!=before.st_ino||after.st_size!=before.st_size||
 after.st_mtim.tv_sec!=before.st_mtim.tv_sec||
 after.st_mtim.tv_nsec!=before.st_mtim.tv_nsec||
 after.st_ctim.tv_sec!=before.st_ctim.tv_sec||
 after.st_ctim.tv_nsec!=before.st_ctim.tv_nsec)valid=0;
 {struct stat named;
  if(lstat(path,&named)!=0||!S_ISREG(named.st_mode)||named.st_nlink!=1||
     named.st_dev!=before.st_dev||named.st_ino!=before.st_ino||
     named.st_size!=before.st_size||
     named.st_mtim.tv_sec!=before.st_mtim.tv_sec||
     named.st_mtim.tv_nsec!=before.st_mtim.tv_nsec||
     named.st_ctim.tv_sec!=before.st_ctim.tv_sec||
     named.st_ctim.tv_nsec!=before.st_ctim.tv_nsec)valid=0;
 }
done:
 if(fclose(file)!=0)valid=0;
 if(valid&&matches==1&&found.active){*out=found;return 1;}
 return 0;
}
