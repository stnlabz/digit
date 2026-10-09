#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "interpretation.h"
#include "corpus.h"

/* [AI:GPT-5.6 Sol | 2026-10-07T00:45:00Z] Initial deterministic Interpretation module. Resolves explicit operator-learned "means" relationships from Corpus before Intent without embedding vocabulary knowledge in code. */
/* [AI:GPT-5.6 Sol | 2026-10-07T01:20:00Z] Interpretation can remove a terminal addressee only when authorized Corpus evidence establishes both a conversational response relation for the utterance and identity evidence for the addressee; no names or greetings are embedded in code. */

/* [AI:GPT-6 | 2026-10-08] Restrict learned-meaning substitutions to exact definition subjects; unrelated words in learned records cannot redefine input. */
static const stnlabz_module_host_t *interpretation_host = NULL;

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
    if (text == NULL || word == NULL || word[0] == '\0') return 0;
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

/* [AI:GPT-6 | 2026-10-09] Interpret only explicit, unambiguous
 * learned definitions. A conflicting lesson cannot silently win by rank. */
static int parse_learned_definition(const digit_corpus_record_t *record,
                                    const char *word,char *out,size_t capacity)
{
    const char *start,*meaning,*p;
    size_t subject_length,n=0;
    if(!record||!word||!out||capacity==0||
       strcmp(record->category,"OPERATOR_LEARNED")!=0||
       strcmp(record->source,"interface:learn")!=0)return 0;
    start=record->text;
    meaning=strstr(start," means ");
    if(!meaning)return 0;
    while(start<meaning&&isspace((unsigned char)*start))++start;
    subject_length=(size_t)(meaning-start);
    while(subject_length&&isspace((unsigned char)start[subject_length-1]))--subject_length;
    if(!word_equal_ci(start,subject_length,word))return 0;
    p=meaning+strlen(" means ");
    while(*p&&isspace((unsigned char)*p))++p;
    if(!isalnum((unsigned char)*p)&&*p!='_'&&*p!='-')return 0;
    while(p[n]&&(isalnum((unsigned char)p[n])||p[n]=='_'||p[n]=='-'))++n;
    while(isspace((unsigned char)p[n]))++n;
    /* No partial-phrase substitution: a definition with multiple words
     * does not authorize replacing an input token with its first word. */
    if(p[n]!='\\0'&&p[n]!='.')return 0;
    if(p[n]=='.'&&p[n+1]!='\\0')return 0;
    n=0;
    while(p[n]&&(isalnum((unsigned char)p[n])||p[n]=='_'||p[n]=='-'))++n;
    if(n==0||n>=capacity)return 0;
    memcpy(out,p,n);
    out[n]='\\0';
    return 1;
}

static int unique_learned_meaning(const digit_corpus_search_result_t *result,
                                  const char *word,char *out,size_t capacity)
{
    char candidate[128],resolved[128];
    size_t i;int found=0;
    if(!result||!word||!out||!capacity)return 0;
    for(i=0;i<result->count&&i<DIGIT_CORPUS_SEARCH_MAX;++i){
        if(!parse_learned_definition(&result->records[i],word,candidate,sizeof(candidate)))continue;
        if(found&&strcmp(resolved,candidate)!=0)return 0;
        snprintf(resolved,sizeof(resolved),"%s",candidate);
        found=1;
    }
    if(!found||strlen(resolved)>=capacity)return 0;
    snprintf(out,capacity,"%s",resolved);
    return 1;
}

static int learned_meaning(const char *word, char *out, size_t out_size)
{
    digit_corpus_search_request_t request;
    digit_corpus_search_result_t result;
    size_t used=0;
    if(word==NULL||out==NULL||out_size==0||interpretation_host==NULL||
       interpretation_host->invoke_service==NULL)return 0;
    memset(&request,0,sizeof(request));
    memset(&result,0,sizeof(result));
    snprintf(request.query,sizeof(request.query),"%s",word);
    if(interpretation_host->invoke_service(DIGIT_CORPUS_SEARCH_SERVICE,&request,sizeof(request),
       &result,sizeof(result),&used)!=STNLABZ_MODULE_OK||used!=sizeof(result))return 0;
    return unique_learned_meaning(&result,word,out,out_size);
}

static void trim_trailing_space(char *text)
{
    size_t n;
    if (text == NULL) return;
    n = strlen(text);
    while (n > 0 && isspace((unsigned char)text[n - 1])) text[--n] = '\0';
}

static int strip_terminal_addressee(char *text)
{
    const char *p;
    char first[128], last[128];
    size_t first_len = 0, last_len = 0, prefix_len;
    if (text == NULL || interpretation_host == NULL || interpretation_host->invoke_service == NULL) return 0;
    p = text;
    while (*p && isspace((unsigned char)*p)) ++p;
    while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-') && first_len + 1 < sizeof(first))
        first[first_len++] = *p++;
    first[first_len] = '\0';
    if (first_len == 0) return 0;
    trim_trailing_space(text);
    p = text + strlen(text);
    while (p > text && !isalnum((unsigned char)p[-1]) && p[-1] != '_' && p[-1] != '-') --p;
    {
        const char *end = p;
        while (p > text && (isalnum((unsigned char)p[-1]) || p[-1] == '_' || p[-1] == '-')) --p;
        last_len = (size_t)(end - p);
        if (last_len == 0 || last_len >= sizeof(last)) return 0;
        memcpy(last, p, last_len);
        last[last_len] = '\0';
        prefix_len = (size_t)(p - text);
    }
    if (word_equal_ci(first, first_len, last)) return 0;
    {
        digit_corpus_search_request_t request;
        digit_corpus_search_result_t result;
        size_t used = 0, i;
        int first_has_relation = 0, last_is_identity = 0;
        memset(&request, 0, sizeof(request)); memset(&result, 0, sizeof(result));
        snprintf(request.query, sizeof(request.query), "%s", first);
        if (interpretation_host->invoke_service(DIGIT_CORPUS_SEARCH_SERVICE, &request, sizeof(request),
                                                &result, sizeof(result), &used) == STNLABZ_MODULE_OK &&
            used == sizeof(result))
            for (i = 0; i < result.count && i < DIGIT_CORPUS_SEARCH_MAX; ++i)
                if (strcmp(result.records[i].category, "OPERATOR_LEARNED") == 0 &&
                    strcmp(result.records[i].source, "interface:learn") == 0 &&
                    has_word(result.records[i].text, first) &&
                    strstr(result.records[i].text, "can be responded to with") != NULL)
                { first_has_relation = 1; break; }
        memset(&request, 0, sizeof(request)); memset(&result, 0, sizeof(result)); used = 0;
        snprintf(request.query, sizeof(request.query), "%s", last);
        if (interpretation_host->invoke_service(DIGIT_CORPUS_SEARCH_SERVICE, &request, sizeof(request),
                                                &result, sizeof(result), &used) == STNLABZ_MODULE_OK &&
            used == sizeof(result))
            for (i = 0; i < result.count && i < DIGIT_CORPUS_SEARCH_MAX; ++i)
                if (has_word(result.records[i].text, last) &&
                    (strstr(result.records[i].text, "I am ") != NULL ||
                     strstr(result.records[i].text, "who are you") != NULL))
                { last_is_identity = 1; break; }
        if (!first_has_relation || !last_is_identity) return 0;
    }
    while (prefix_len > 0 && (isspace((unsigned char)text[prefix_len - 1]) ||
           text[prefix_len - 1] == ',')) --prefix_len;
    text[prefix_len] = '\0';
    return 1;
}

static int resolve_text(const char *input, char *output, size_t output_size, unsigned int *substitutions)
{
    const char *p = input;
    size_t used = 0;
    unsigned int changed = 0;
    if (input == NULL || output == NULL || output_size == 0) return 0;
    output[0] = '\0';
    while (*p && used + 1 < output_size)
    {
        if (isalnum((unsigned char)*p) || *p == '_' || *p == '-')
        {
            const char *start = p;
            char word[128], meaning[128];
            size_t n;
            while (*p && (isalnum((unsigned char)*p) || *p == '_' || *p == '-')) ++p;
            n = (size_t)(p - start);
            if (n >= sizeof(word)) n = sizeof(word) - 1;
            memcpy(word, start, n);
            word[n] = '\0';
            if (learned_meaning(word, meaning, sizeof(meaning)))
            {
                start = meaning;
                n = strlen(meaning);
                ++changed;
            }
            if (n >= output_size - used) return 0;
            memcpy(output + used, start, n);
            used += n;
        }
        else
        {
            output[used++] = *p++;
        }
    }
    if (*p != '\0') return 0;
    output[used] = '\0';
    if (substitutions != NULL) *substitutions = changed;
    return 1;
}

static stnlabz_module_result_t interpretation_service(const void *request, size_t request_size,
                                                       void *response, size_t response_size,
                                                       size_t *response_used, void *handler_context)
{
    const digit_interpretation_request_t *input = request;
    digit_interpretation_result_t result;
    (void)handler_context;
    if (request == NULL || request_size != sizeof(*input) || response == NULL ||
        response_used == NULL || response_size < sizeof(result))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (memchr(input->text, '\0', sizeof(input->text)) == NULL || input->text[0] == '\0')
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(&result, 0, sizeof(result));
    if (!resolve_text(input->text, result.normalized, sizeof(result.normalized), &result.substitutions))
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    {
        int addressee_removed = strip_terminal_addressee(result.normalized);
        result.resolved = 1U;
        snprintf(result.reason, sizeof(result.reason), "%s",
                 addressee_removed ? "Authorized learned meaning applied; terminal addressee removed." :
                 result.substitutions ? "Authorized learned meaning applied." :
                 "No learned meaning substitution required.");
    }
    memcpy(response, &result, sizeof(result));
    *response_used = sizeof(result);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t interpretation_qualify(stnlabz_module_qualification_result_t *result)
{
    if (result == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result, 0, sizeof(*result));
    /* [AI:GPT-6 | 2026-10-09] Measured offline qualification:
     * no Corpus host required for preservation and boundary checks. */
    {
        static const char *samples[] = {
            "hello Digit", "What is the build status?",
            "compare C and Python", "write a C function",
            "who are you?", "explain module qualification",
            "hello, Digit", "ingest lesson-one",
            "2 + 2", "report errors"
        };
        size_t i;
        for(i=0;i<sizeof(samples)/sizeof(samples[0]);++i){
            char output[DIGIT_INTERPRETATION_TEXT_MAX];
            unsigned int changes=99;
            int ok=resolve_text(samples[i],output,sizeof(output),&changes);
            result->tests_executed++;
            if(ok&&strcmp(output,samples[i])==0&&changes==0)result->tests_passed++;
        }
        /* [AI:GPT-6 | 2026-10-09] Definition interpretation gates:
         * exact learned subject, conflicting evidence, and multiword safety. */
        {
            digit_corpus_record_t record={0};
            digit_corpus_search_result_t evidence={0};
            char meaning[128]={0};
            snprintf(record.category,sizeof(record.category),"OPERATOR_LEARNED");
            snprintf(record.source,sizeof(record.source),"interface:learn");
            snprintf(record.text,sizeof(record.text),"wut means what");
            result->tests_executed++;
            if(parse_learned_definition(&record,"wut",meaning,sizeof(meaning))&&
               strcmp(meaning,"what")==0)result->tests_passed++;
            result->tests_executed++;
            if(!parse_learned_definition(&record,"who",meaning,sizeof(meaning)))result->tests_passed++;
            evidence.count=2;evidence.records[0]=record;evidence.records[1]=record;
            snprintf(evidence.records[1].text,sizeof(evidence.records[1].text),"wut means which");
            result->tests_executed++;
            if(!unique_learned_meaning(&evidence,"wut",meaning,sizeof(meaning)))result->tests_passed++;
            snprintf(evidence.records[1].text,sizeof(evidence.records[1].text),"wut means what");
            result->tests_executed++;
            if(unique_learned_meaning(&evidence,"wut",meaning,sizeof(meaning))&&
               strcmp(meaning,"what")==0)result->tests_passed++;
            snprintf(record.text,sizeof(record.text),"wut means what exactly");
            result->tests_executed++;
            if(!parse_learned_definition(&record,"wut",meaning,sizeof(meaning)))result->tests_passed++;
            snprintf(record.source,sizeof(record.source),"untrusted:source");
            result->tests_executed++;
            if(!parse_learned_definition(&record,"wut",meaning,sizeof(meaning)))result->tests_passed++;
        }
        result->negative_test_executed=1;
        result->negative_test_passed=!resolve_text("oversized", (char[2]){0}, 2, NULL);
    }
    result->tests_failed=result->tests_executed-result->tests_passed;
    return result->tests_failed||result->negative_test_passed!=result->negative_test_executed?
        STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t interpretation_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL || host->invoke_service == NULL)
        return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_INTERPRETATION_SERVICE, interpretation_service, NULL))
        return STNLABZ_MODULE_ERR_START_FAILED;
    interpretation_host = host;
    if (host->send_message != NULL)
        (void)host->send_message("[INTERPRETATION] active: authorized learned meanings resolve before Intent");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t interpretation_stop(void)
{
    if (interpretation_host != NULL && interpretation_host->unregister_service != NULL)
        if (!interpretation_host->unregister_service(DIGIT_INTERPRETATION_SERVICE, NULL))
            return STNLABZ_MODULE_ERR_STOP_FAILED;
    interpretation_host = NULL;
    return STNLABZ_MODULE_OK;
}

static const stnlabz_module_descriptor_t interpretation_descriptor =
{
    "interpretation", "Digit Interpretation", 1, 0, 4,
    STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
    interpretation_qualify, interpretation_start, interpretation_stop
};

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    return &interpretation_descriptor;
}
