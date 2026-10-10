#ifndef DIGIT_MEMORY_RESPONSE_H
#define DIGIT_MEMORY_RESPONSE_H
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "memory.h"

/* Bounded presentation of a verified memory; never adds unsupported facts. */
static int digit_memory_natural_reply(const char *saved,char *out,size_t cap){
 char name[64]={0};size_t i=0,pos=0,n=0;const char *p=saved;
 int first=1;
 if(!saved||!out||cap<64||!memchr(saved,0,DIGIT_MEMORY_TEXT_MAX)||!saved[0])return 0;
 out[0]=0;
 if(!strncmp(p,"I am ",5)){
  const char *q=p+5;
  while(isalpha((unsigned char)*q)||*q=='-'||*q=='\''){
   if(n+1>=sizeof(name))break;
   name[n++]=*q++;
  }
  if(*q!=','&&*q!='.'&&*q!=';'&&*q!=' '){name[0]=0;}
  if(n<2)name[0]=0;
 }
 if(name[0]){
  int used=snprintf(out,cap,"Hi %s! You told me that ",name);
  if(used<0||(size_t)used>=cap)return 0;
  pos=(size_t)used;
 }else{
  const char *intro="You told me that ";
  if(strlen(intro)>=cap)return 0;
  strcpy(out,intro);pos=strlen(out);
 }
 while(*p){
  const char *replace=NULL;size_t consume=0,k;
  if((first||isspace((unsigned char)p[-1])||p[-1]==',')&&
     !strncmp(p,"I am ",5)){replace="you are ";consume=5;}
  else if((first||isspace((unsigned char)p[-1])||p[-1]==',')&&
          !strncmp(p,"I ",2)){replace="you ";consume=2;}
  else if((first||isspace((unsigned char)p[-1])||p[-1]==',')&&
          !strncmp(p,"my ",3)){replace="your ";consume=3;}
  if(replace){
   k=strlen(replace);
   if(pos+k>=cap)return 0;
   memcpy(out+pos,replace,k);pos+=k;p+=consume;
  }else{
   if(pos+1>=cap)return 0;
   out[pos++]=*p++;
  }
  first=0;
 }
 if(pos&&out[pos-1]!='.'&&out[pos-1]!='!'&&out[pos-1]!='?'){
  if(pos+1>=cap)return 0;
  out[pos++]='.';
 }
 out[pos]=0;
 return 1;
}
#endif
