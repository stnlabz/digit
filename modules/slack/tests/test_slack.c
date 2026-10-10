#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
/* Include module to exercise internal config and event admission without network. */
#include "../src/slack.c"
static void test_config(void){
 char name[]="/tmp/digit-slack-test-XXXXXX";
 const char *sample="enabled=true\nworkspace_id=T123\napp_token=xapp-test\nbot_token=xoxb-test\n";
 slack_config_t loaded={0};FILE *f;int fd=mkstemp(name);
 assert(fd>=0);
 f=fdopen(fd,"w");assert(f);
 assert(fputs(sample,f)>=0);assert(!fclose(f));
 assert(chmod(name,0600)==0);
 assert(load_config(name,&loaded));
 assert(loaded.enabled&&strcmp(loaded.workspace,"T123")==0);
 assert(chmod(name,0644)==0);
 assert(!load_config(name,&loaded));
 unlink(name);erase(&loaded,sizeof(loaded));
}
static void test_fail_closed(void){
 struct json_object *j;
 assert(!load_config("/file/not/here",&cfg));
 assert(!bounded(bot_id,3,"B12345"));
 j=json_tokener_parse("{\"payload\":{\"event\":{\"type\":\"app_mention\",\"user\":\"U1\",\"channel\":\"C1\",\"text\":\"hello\"}},\"team_id\":\"OTHER\"}");
 assert(j);
 snprintf(cfg.workspace,sizeof(cfg.workspace),"T123");
 assert(!process_event(j));
 json_object_put(j);
}
int main(void){
 stnlabz_module_qualification_result_t result;
 assert(stnlabz_module_get_descriptor()->qualify(&result)==STNLABZ_MODULE_OK);
 assert(result.tests_executed>=10&&result.tests_failed==0);
 test_config();test_fail_closed();
 puts("Digit Slack module offline tests passed");
 return 0;
}
