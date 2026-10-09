#include <stdio.h>
#include <string.h>
#include "session_store.h"

/* [AI:GPT-6 | 2026-10-08] Qualification and negative tests for session lifecycle. */
static unsigned int count=0, failed=0;
static void check(int pass, const char *name)
{
    ++count;
    if(pass) printf("PASS %02u - %s\n",count,name);
    else {++failed;printf("FAIL %02u - %s\n",count,name);}
}
int main(void)
{
    digit_session_store_t store;
    char token[DIGIT_SESSION_TOKEN_SIZE],second[DIGIT_SESSION_TOKEN_SIZE];
    char identity[DIGIT_SESSION_ID_SIZE],small[2];
    time_t now=1000;
    digit_session_store_init(&store);
    check(!digit_session_resolve(&store,"",now,identity,sizeof(identity)),"empty token denied");
    check(!digit_session_issue(&store,"ezra",0,now,token),"unverified authentication denied");
    check(!digit_session_issue(&store,"",1,now,token),"empty identity denied");
    check(!digit_session_issue(&store,"ezra",1,0,token),"invalid timestamp denied");
    check(digit_session_issue(&store,"ezra",1,now,token),"verified identity issues token");
    check(strlen(token)==64U,"token is 256-bit hex");
    check(digit_session_resolve(&store,token,now,identity,sizeof(identity)) &&
          strcmp(identity,"ezra")==0,"resolve binds token to identity");
    check(!digit_session_resolve(&store,token,now,small,sizeof(small)),"short output rejected");
    check(!digit_session_resolve(&store,token,now+DIGIT_SESSION_LIFETIME,identity,sizeof(identity)),"expired token denied");
    check(!digit_session_resolve(&store,"invalid",now,identity,sizeof(identity)),"malformed token denied");
    check(digit_session_issue(&store,"poe",1,now,second),"second identity issues token");
    check(strcmp(token,second)!=0,"distinct sessions receive distinct tokens");
    check(digit_session_resolve(&store,second,now,identity,sizeof(identity)) &&
          strcmp(identity,"poe")==0,"second token resolves separate identity");
    check(digit_session_revoke_identity(&store,"ezra")==1,"revoke all sessions for identity");
    check(!digit_session_resolve(&store,token,now,identity,sizeof(identity)),"revoked identity denied");
    check(digit_session_resolve(&store,second,now,identity,sizeof(identity)),"other identity preserved");
    /* [AI:GPT-6 | 2026-10-09] Preferred names belong only to a live session. */
    {
        size_t i;int found=0;
        for(i=0;i<DIGIT_SESSION_CAPACITY;i++){
            if(store.entries[i].active&&strcmp(store.entries[i].token,second)==0){
                snprintf(store.entries[i].preferred_name,sizeof(store.entries[i].preferred_name),"Poe");
                found=strcmp(store.entries[i].preferred_name,"Poe")==0;
                break;
            }
        }
        check(found,"preferred name stays within resolved session");
    }
    check(digit_session_revoke(&store,second),"single session revocation");
    {
        size_t i;int erased=1;
        for(i=0;i<DIGIT_SESSION_CAPACITY;i++)
            if(strcmp(store.entries[i].preferred_name,"Poe")==0)erased=0;
        check(erased,"revoke erases volatile preferred name");
    }
    check(!digit_session_resolve(&store,second,now,identity,sizeof(identity)),"revoked token denied");
    check(!digit_session_revoke(&store,second),"duplicate revocation rejected");
    check(!digit_session_issue(NULL,"ezra",1,now,token),"null store denied");
    check(!digit_session_issue(&store,"ezra",1,now,NULL),"null token denied");
    check(!digit_session_resolve(NULL,token,now,identity,sizeof(identity)),"null store resolve denied");
    check(!digit_session_resolve(&store,token,now,NULL,0),"null identity output denied");
    check(digit_session_revoke_identity(&store,NULL)==0,"null identity revocation safe");
    printf("\nSession tests: %u executed, %u failed\n",count,failed);
    return failed?1:0;
}
