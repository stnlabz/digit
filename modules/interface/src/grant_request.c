#include <string.h>
#include "grant_request.h"
/* [AI:GPT-6 | 2026-10-09] Exact four-field parser, syntax only.
 * Authorization is a separate mandatory check. */
static int identifier(const char *s,size_t n){
 size_t i;
 if(n==0||n>=64)return 0;
 for(i=0;i<n;++i){
  unsigned char c=(unsigned char)s[i];
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;
 }
 return 1;
}
int digit_grant_request_parse(const char *text,digit_grant_request_t *out){
 digit_grant_request_t value={0};
 char *dest[4]={value.organization,value.project,value.channel,value.user};
 const char *start,*end;
 size_t i,length;
 if(!text||!out)return 0;
 memset(out,0,sizeof(*out));
 start=text;
 for(i=0;i<4;++i){
  end=i<3?strchr(start,9):start+strlen(start);
  if(!end)return 0;
  length=(size_t)(end-start);
  if(!identifier(start,length))return 0;
  memcpy(dest[i],start,length);
  dest[i][length]=0;
  start=end+1;
 }
 *out=value;
 return 1;
}
