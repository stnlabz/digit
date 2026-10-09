#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "intent.h"
#include "../../corpus/includes/corpus.h"

/* [AI:GPT-5.6 Sol | 2026-10-06T22:41:00Z] Initial deterministic Intent implementation. Interprets request purpose and target only; it contains no subject-specific knowledge or answers. */
/* [AI:GPT-5.6 Sol | 2026-10-06T23:03:00Z] Qualification now executes the deterministic interpreter and reports measured pass/fail results instead of declared results. */
/* [AI:GPT-5.6 Sol | 2026-10-07T00:52:00Z] Knowledge operations are recognized inside conversational framing instead of requiring the operation verb to be the first token. Subject extraction begins after the established operation. */
/* [AI:GPT-5.6 Sol | 2026-10-07T01:50:00Z] COMPARE now carries its established comparison subject expression in the Intent envelope so downstream retrieval does not rank the operation word as evidence. */

/* [AI:GPT-6 | 2026-10-08] Require exact learned-definition subject before " means " to prevent unrelated Corpus text from rewriting requests. */
static const stnlabz_module_host_t *intent_host = NULL;

static int word_equal_ci(const char *start, size_t length, const char *word)
{
    size_t i;
    if (start == NULL || word == NULL || strlen(word) != length) return 0;
    for (i = 0; i < length; ++i)
        if (tolower((unsigned char)start[i]) != tolower((unsigned char)word[i])) return 0;
    return 1;
}

static int has_word(const char *text, const char *word)
{
    const char *p = text;
    if (text == NULL || word == NULL) return 0;
    while (*p)
    {
        const char *start;
        size_t length;
        while (*p && !isalnum((unsigned char)*p) && *p != '_' && *p != '-') ++p;
        start = p;
        while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-')) ++p;
        length = (size_t)(p - start);
        if (length > 0 && word_equal_ci(start, length, word)) return 1;
    }
    return 0;
}

/* [AI:GPT-6 | 2026-10-09] Command verbs embedded in knowledge
 * questions are not operator instructions. */
static int starts_with_command(const char *text,const char *const *words,size_t count){
 const char *p=text;size_t i;
 if(!p)return 0;
 while(*p&&isspace((unsigned char)*p))++p;
 if(strncmp(p,"please ",7)==0||strncmp(p,"Please ",7)==0)p+=7;
 for(i=0;i<count;i++){
  size_t n=strlen(words[i]);
  if(word_equal_ci(p,n,words[i])&&(!p[n]||isspace((unsigned char)p[n])||p[n]==':'))return 1;
 }
 return 0;
}
static int any_word(const char *text, const char *const *words, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (has_word(text, words[i])) return 1;
    return 0;
}

static int find_word(const char *text, const char *word, const char **after)
{
    const char *p = text;
    if (text == NULL || word == NULL) return 0;
    while (*p)
    {
        const char *start;
        size_t length;
        while (*p && !isalnum((unsigned char)*p) && *p != '_' && *p != '-') ++p;
        start = p;
        while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-')) ++p;
        length = (size_t)(p - start);
        if (length > 0 && word_equal_ci(start, length, word))
        {
            if (after != NULL) *after = p;
            return 1;
        }
    }
    return 0;
}

static void semantic_subject(const char *after, char *subject, size_t size)
{
    size_t n;
    if (subject == NULL || size == 0) return;
    subject[0] = '\0';
    if (after == NULL) return;
    while (*after && !isalnum((unsigned char)*after) && *after != '_' && *after != '-') ++after;
    n = strlen(after);
    while (n > 0 && !isalnum((unsigned char)after[n - 1]) && after[n - 1] != '_' && after[n - 1] != '-') --n;
    if (n >= size) n = size - 1;
    memcpy(subject, after, n);
    subject[n] = '\0';
}

static int learned_word(const char *word,char *out,size_t cap)
{
    digit_corpus_search_request_t q; digit_corpus_search_result_t r; size_t used=0,i;
    if(!word||!out||!cap||!intent_host||!intent_host->invoke_service)return 0;
    memset(&q,0,sizeof(q)); memset(&r,0,sizeof(r)); snprintf(q.query,sizeof(q.query),"%s",word);
    if(intent_host->invoke_service(DIGIT_CORPUS_SEARCH_SERVICE,&q,sizeof(q),&r,sizeof(r),&used)!=STNLABZ_MODULE_OK||used!=sizeof(r))return 0;
    for(i=0;i<r.count&&i<DIGIT_CORPUS_SEARCH_MAX;i++){
        const char *p,*m; size_t n=0;
        const char *start=r.records[i].text; size_t prefix_length;
        if(strcmp(r.records[i].category,"OPERATOR_LEARNED")!=0)continue;
        m=strstr(start," means "); if(!m)continue;
        while(start<m&&isspace((unsigned char)*start))++start;
        prefix_length=(size_t)(m-start);
        while(prefix_length>0&&isspace((unsigned char)start[prefix_length-1]))--prefix_length;
        if(!word_equal_ci(start,prefix_length,word))continue;
        p=m+7;
        while(*p&&(!isalnum((unsigned char)*p)&&*p!='_'&&*p!='-'))p++;
        while(p[n]&&(isalnum((unsigned char)p[n])||p[n]=='_'||p[n]=='-')&&n+1<cap)n++;
        if(n){memcpy(out,p,n);out[n]=0;return 1;}
    } return 0;
}
/* [AI:GPT-6 | 2026-10-09] Never classify silently truncated requests.
 * If a learned substitution expands past the ABI boundary, fail closed. */
static int learned_text(const char *in,char *out,size_t cap)
{
 const char *p=in;size_t used=0;
 if(!in||!out||cap==0)return 0;
 out[0]=0;
 while(*p){
  if(isalnum((unsigned char)*p)||*p=='_'||*p=='-'){
   const char *start=p,*value;
   char word[128],replacement[128];
   size_t length,copy_length;
   while(*p&&(isalnum((unsigned char)*p)||*p=='_'||*p=='-'))++p;
   length=(size_t)(p-start);
   value=start;copy_length=length;
   if(length<sizeof(word)){
    memcpy(word,start,length);word[length]=0;
    if(learned_word(word,replacement,sizeof(replacement))){
     value=replacement;copy_length=strlen(replacement);
    }
   }
   if(copy_length>=cap-used)return 0;
   memcpy(out+used,value,copy_length);used+=copy_length;
  }else{
   if(used+1>=cap)return 0;
   out[used++]=*p++;
  }
 }
 out[used]=0;
 return 1;
}

static void set_result(digit_intent_result_t *result, digit_intent_class_t intent,
                       digit_intent_target_t target, unsigned int established,
                       const char *reason)
{
    result->intent = intent;
    result->target = target;
    result->established = established;
    snprintf(result->reason, sizeof(result->reason), "%s", reason);
}

void digit_intent_interpret(const char *text, digit_intent_result_t *result)
{
    static const char *const action_words[] = {"create","build","write","generate","make","implement","produce","fix","remove","delete","install","uninstall","update","patch","ingest"};
    static const char *const status_words[] = {"status","errors","error","alerts","alert","broken","health","running","failures","failure"};
    static const char *const social_words[] = {"hi","hello","hey","morning","afternoon","evening","thanks","thank","sorry","ouch","paws"};
    static const char *const compare_words[] = {"compare","versus","difference","differences"};
    const char *after = NULL;
    char interpreted[DIGIT_INTENT_TEXT_MAX];
    int action, status, explain, define, compare, why, how, fact, social;
    unsigned int operational_count;
    if (result == NULL) return;
    memset(result, 0, sizeof(*result));
    if (text == NULL || text[0] == '\0') { set_result(result,DIGIT_INTENT_UNKNOWN,DIGIT_INTENT_TARGET_UNKNOWN,0U,"Intent is not deterministically established."); return; }
    if(strlen(text)>=sizeof(interpreted) ||
       !learned_text(text,interpreted,sizeof(interpreted))){
        set_result(result,DIGIT_INTENT_UNKNOWN,DIGIT_INTENT_TARGET_UNKNOWN,0U,
                   "Request exceeds the interpretation boundary.");
        return;
    }
    text=interpreted;
    /* [AI:GPT-6 | 2026-10-09] A bounded solve-equation request is
     * an arithmetic knowledge query, not a privileged executable action. */
    {
        const char *p=text;
        if(strncasecmp(p,"digit ",6)==0)p+=6;
        if(strncasecmp(p,"solve ",6)==0){
            const char *q=p+6;int equation=0,variable=0,valid=1;
            while(*q){
                unsigned char c=(unsigned char)*q++;
                if(c=='=')equation=1;
                else if(c=='x'||c=='X')variable=1;
                else if(!(isdigit(c)||isspace(c)||c=='.'||c=='+'||
                          c=='-'||c=='*'||c=='/'||c=='^'||c=='('||c==')'||c=='?'))
                    valid=0;
            }
            if(equation&&variable&&valid){
                set_result(result,DIGIT_INTENT_FACT,DIGIT_INTENT_TARGET_KNOWLEDGE,
                           1U,"Bounded algebra equation request.");
                return;
            }
        }
    }
    action=starts_with_command(text,action_words,sizeof(action_words)/sizeof(action_words[0]));
    status=any_word(text,status_words,sizeof(status_words)/sizeof(status_words[0]));
    explain=find_word(text,"explain",&after);
    define=find_word(text,"define",NULL);
    compare=any_word(text,compare_words,sizeof(compare_words)/sizeof(compare_words[0]));
    why=has_word(text,"why");
    how=has_word(text,"how");
    fact=has_word(text,"what")||has_word(text,"who");
    social=any_word(text,social_words,sizeof(social_words)/sizeof(social_words[0]));
    /* Question words framing an explicit knowledge operation are not conflicting intents. */
    if(explain||define||compare){status=0;why=0;how=0;fact=0;}
    if(fact&&!action){status=0;}
    operational_count=(unsigned int)action+(unsigned int)status+(unsigned int)explain+(unsigned int)define+(unsigned int)compare+(unsigned int)why+(unsigned int)how+(unsigned int)fact;
    if(operational_count>1U){set_result(result,DIGIT_INTENT_AMBIGUOUS,DIGIT_INTENT_TARGET_UNKNOWN,0U,"Request contains conflicting operational meanings.");return;}
    if(action){set_result(result,DIGIT_INTENT_ACTION,DIGIT_INTENT_TARGET_CAPABILITY,1U,"Request directs Digit to perform or change something.");return;}
    if(status){set_result(result,DIGIT_INTENT_STATUS,DIGIT_INTENT_TARGET_RUNTIME,1U,"Request asks about current operational state.");return;}
    if(explain){semantic_subject(after,result->subject,sizeof(result->subject));set_result(result,DIGIT_INTENT_EXPLAIN,DIGIT_INTENT_TARGET_KNOWLEDGE,result->subject[0]!='\0',"Request asks for an explanation.");return;}
    if(define){find_word(text,"define",&after);semantic_subject(after,result->subject,sizeof(result->subject));set_result(result,DIGIT_INTENT_DEFINE,DIGIT_INTENT_TARGET_KNOWLEDGE,result->subject[0]!='\0',"Request asks for a definition.");return;}
    if(compare){if(find_word(text,"compare",&after)){semantic_subject(after,result->subject,sizeof(result->subject));}else{snprintf(result->subject,sizeof(result->subject),"%s",text);}set_result(result,DIGIT_INTENT_COMPARE,DIGIT_INTENT_TARGET_KNOWLEDGE,result->subject[0]!='\0',"Request asks for a comparison.");return;}
    if(why){set_result(result,DIGIT_INTENT_WHY,DIGIT_INTENT_TARGET_KNOWLEDGE,1U,"Request asks for a supported reason or cause.");return;}
    if(how){set_result(result,DIGIT_INTENT_HOW,DIGIT_INTENT_TARGET_KNOWLEDGE,1U,"Request asks how something works or is done.");return;}
    if(fact){set_result(result,DIGIT_INTENT_FACT,DIGIT_INTENT_TARGET_KNOWLEDGE,1U,"Request asks for factual knowledge.");return;}
    if(social){set_result(result,DIGIT_INTENT_CONVERSATION,DIGIT_INTENT_TARGET_SOCIAL,1U,"Request is conversational rather than an operational or knowledge task.");return;}
    set_result(result,DIGIT_INTENT_UNKNOWN,DIGIT_INTENT_TARGET_UNKNOWN,0U,"Intent is not deterministically established.");
}

const char *digit_intent_class_string(digit_intent_class_t intent)
{
    switch (intent)
    {
        case DIGIT_INTENT_CONVERSATION: return "CONVERSATION";
        case DIGIT_INTENT_FACT: return "FACT";
        case DIGIT_INTENT_DEFINE: return "DEFINE";
        case DIGIT_INTENT_EXPLAIN: return "EXPLAIN";
        case DIGIT_INTENT_COMPARE: return "COMPARE";
        case DIGIT_INTENT_WHY: return "WHY";
        case DIGIT_INTENT_HOW: return "HOW";
        case DIGIT_INTENT_STATUS: return "STATUS";
        case DIGIT_INTENT_ACTION: return "ACTION";
        case DIGIT_INTENT_AMBIGUOUS: return "AMBIGUOUS";
        default: return "UNKNOWN";
    }
}

const char *digit_intent_target_string(digit_intent_target_t target)
{
    switch (target)
    {
        case DIGIT_INTENT_TARGET_SOCIAL: return "SOCIAL";
        case DIGIT_INTENT_TARGET_KNOWLEDGE: return "KNOWLEDGE";
        case DIGIT_INTENT_TARGET_RUNTIME: return "RUNTIME";
        case DIGIT_INTENT_TARGET_CAPABILITY: return "CAPABILITY";
        default: return "UNKNOWN";
    }
}

static stnlabz_module_result_t intent_service(const void *request, size_t request_size,
                                              void *response, size_t response_size,
                                              size_t *response_used, void *handler_context)
{
    const digit_intent_request_t *input = request;
    digit_intent_result_t result;
    (void)handler_context;
    if (request == NULL || request_size != sizeof(*input) || response == NULL ||
        response_used == NULL || response_size < sizeof(result))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->text, '\0', sizeof(input->text)) == NULL || input->text[0] == '\0')
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    digit_intent_interpret(input->text, &result);
    memcpy(response, &result, sizeof(result));
    *response_used = sizeof(result);
    return STNLABZ_MODULE_OK;
}

static int qualification_case(const char *text, digit_intent_class_t expected_intent,
                              digit_intent_target_t expected_target,
                              unsigned int expected_established)
{
    digit_intent_result_t interpreted;
    digit_intent_interpret(text, &interpreted);
    return interpreted.intent == expected_intent &&
           interpreted.target == expected_target &&
           interpreted.established == expected_established;
}

static stnlabz_module_result_t intent_qualify(stnlabz_module_qualification_result_t *result)
{
    static const struct
    {
        const char *text;
        digit_intent_class_t intent;
        digit_intent_target_t target;
        unsigned int established;
    } cases[] =
    {
        {"hello Digit", DIGIT_INTENT_CONVERSATION, DIGIT_INTENT_TARGET_SOCIAL, 1U},
        {"what is the first General Order", DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"define deterministic behavior", DIGIT_INTENT_DEFINE, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"explain deterministic behavior", DIGIT_INTENT_EXPLAIN, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"Explain what running on fumes means in your own words.", DIGIT_INTENT_EXPLAIN, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"Explain what it means when a server is toast without using the word ruined.", DIGIT_INTENT_EXPLAIN, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"compare these two implementations", DIGIT_INTENT_COMPARE, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"why did qualification fail", DIGIT_INTENT_WHY, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"how does hotload work", DIGIT_INTENT_HOW, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"report current errors", DIGIT_INTENT_STATUS, DIGIT_INTENT_TARGET_RUNTIME, 1U},
        {"solve x^2 - 5*x + 6 = 0", DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"solve 3*x - 6 = 0", DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"solve x + 1 = x + 2", DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"build a module", DIGIT_INTENT_ACTION, DIGIT_INTENT_TARGET_CAPABILITY, 1U},
        {"What is the build status?", DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"Please build a module", DIGIT_INTENT_ACTION, DIGIT_INTENT_TARGET_CAPABILITY, 1U},
        {"ingest lesson-one", DIGIT_INTENT_ACTION, DIGIT_INTENT_TARGET_CAPABILITY, 1U},
        {"What is the build status?", DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"What does update mean?", DIGIT_INTENT_FACT, DIGIT_INTENT_TARGET_KNOWLEDGE, 1U},
        {"flibbertigibbet", DIGIT_INTENT_UNKNOWN, DIGIT_INTENT_TARGET_UNKNOWN, 0U}
    };
    size_t i;
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        ++result->tests_executed;
        if (qualification_case(cases[i].text, cases[i].intent, cases[i].target,
                               cases[i].established))
            ++result->tests_passed;
    }
    result->tests_failed = result->tests_executed - result->tests_passed;
    result->negative_test_executed = 1;
    result->negative_test_passed =
        qualification_case("flibbertigibbet", DIGIT_INTENT_UNKNOWN,
                           DIGIT_INTENT_TARGET_UNKNOWN, 0U);
    return result->tests_failed == 0 && result->negative_test_passed
        ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_QUALIFICATION;
}

static stnlabz_module_result_t intent_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_INTENT_SERVICE, intent_service, NULL))
        return STNLABZ_MODULE_ERR_START_FAILED;
    intent_host = host;
    if (host->send_message != NULL)
        (void)host->send_message("[INTENT] module active: deterministic intent.interpret registered");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t intent_stop(void)
{
    if (intent_host != NULL && intent_host->unregister_service != NULL)
        if (!intent_host->unregister_service(DIGIT_INTENT_SERVICE, NULL))
            return STNLABZ_MODULE_ERR_STOP_FAILED;
    intent_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t intent_descriptor =
{
    "intent", "Digit Intent", 1, 1, 7,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    intent_qualify, intent_start, intent_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &intent_descriptor;
}
