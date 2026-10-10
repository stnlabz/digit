#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "validator.h"

#define VALIDATOR_TERM_MAX 64
#define VALIDATOR_TERM_COUNT 64

static const stnlabz_module_host_t *validator_host = NULL;

static int contains_ci(const char *text, const char *needle)
{
    size_t i, j, n;
    if (text == NULL || needle == NULL || needle[0] == '\0') return 0;
    n = strlen(needle);
    for (i = 0; text[i] != '\0'; ++i) {
        for (j = 0; j < n && text[i + j] != '\0'; ++j)
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)needle[j])) break;
        if (j == n) return 1;
    }
    return 0;
}

static int stopword(const char *word)
{
    static const char *words[] = {
        "a","an","and","are","as","at","be","been","but","by","can","could","did","do","does",
        "for","from","had","has","have","how","i","if","in","into","is","it","its","may","must",
        "of","on","or","should","that","the","their","then","there","these","they","this","to",
        "was","were","what","when","where","which","who","why","will","with","would","your","you"
    };
    size_t i;
    for (i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
        if (strcmp(word, words[i]) == 0) return 1;
    return 0;
}

static size_t collect_terms(const char *text, char out[VALIDATOR_TERM_COUNT][VALIDATOR_TERM_MAX])
{
    char word[VALIDATOR_TERM_MAX];
    size_t count = 0, w = 0, i;
    unsigned char ch;
    if (text == NULL) return 0;
    for (i = 0;; ++i) {
        ch = (unsigned char)text[i];
        if (isalnum(ch) || ch == '_' || ch == '-') {
            if (w + 1 < sizeof(word)) word[w++] = (char)tolower(ch);
        } else if (w > 0) {
            size_t j;
            int duplicate = 0;
            word[w] = '\0';
            if (!stopword(word)) {
                for (j = 0; j < count; ++j)
                    if (strcmp(out[j], word) == 0) { duplicate = 1; break; }
                if (!duplicate && count < VALIDATOR_TERM_COUNT) {
                    snprintf(out[count], VALIDATOR_TERM_MAX, "%s", word);
                    ++count;
                }
            }
            w = 0;
        }
        if (ch == '\0') break;
    }
    return count;
}

static int has_term(char list[VALIDATOR_TERM_COUNT][VALIDATOR_TERM_MAX], size_t count, const char *term)
{
    size_t i;
    for (i = 0; i < count; ++i) if (strcmp(list[i], term) == 0) return 1;
    return 0;
}

static void trim_copy(const char *src, char *dst, size_t size)
{
    const char *start, *end;
    size_t n;
    if (dst == NULL || size == 0) return;
    dst[0] = '\0';
    if (src == NULL) return;
    start = src;
    while (*start && isspace((unsigned char)*start)) ++start;
    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) --end;
    n = (size_t)(end - start);
    if (n >= size) n = size - 1;
    memcpy(dst, start, n);
    dst[n] = '\0';
}

static void replace_word(char *text, size_t size, const char *bad, const char *good)
{
    char work[DIGIT_VALIDATOR_TEXT_MAX];
    char *p;
    size_t prefix;
    if (text == NULL || bad == NULL || good == NULL) return;
    p = strstr(text, bad);
    if (p == NULL) return;
    prefix = (size_t)(p - text);
    if (prefix >= sizeof(work)) return;
    snprintf(work, sizeof(work), "%.*s%s%s", (int)prefix, text, good, p + strlen(bad));
    snprintf(text, size, "%s", work);
}

static void canonical_text(const char *src, char *dst, size_t size)
{
    size_t i, o = 0;
    int pending_space = 0;
    if (dst == NULL || size == 0) return;
    dst[0] = '\0';
    if (src == NULL) return;
    for (i = 0; src[i] != '\0' && o + 1 < size; ++i) {
        unsigned char ch = (unsigned char)src[i];
        if (isalnum(ch)) {
            if (pending_space && o > 0 && o + 1 < size) dst[o++] = ' ';
            dst[o++] = (char)tolower(ch);
            pending_space = 0;
        } else if (o > 0) pending_space = 1;
    }
    dst[o] = '\0';
}

static int nearly_echo(const char *input, const char *candidate)
{
    char a[DIGIT_VALIDATOR_TEXT_MAX], b[DIGIT_VALIDATOR_TEXT_MAX];
    canonical_text(input, a, sizeof(a));
    canonical_text(candidate, b, sizeof(b));
    if (a[0] == '\0' || b[0] == '\0') return 0;
    if (strcmp(a, b) == 0) return 1;
    if (strlen(a) >= 12 && strstr(b, a) != NULL && strlen(b) <= strlen(a) + 24) return 1;
    if (strlen(b) >= 12 && strstr(a, b) != NULL && strlen(a) <= strlen(b) + 24) return 1;
    return 0;
}

static int asks_explain(const char *text)
{
    return contains_ci(text, "explain") || contains_ci(text, "describe") || contains_ci(text, "walk me through") || contains_ci(text, "break down");
}

static int weak_explanation(const char *candidate)
{
    size_t n;
    int sentence_marks = 0;
    const char *p;
    if (candidate == NULL) return 1;
    n = strlen(candidate);
    if (n < 55) return 1;
    for (p = candidate; *p; ++p) if (*p == '.' || *p == ';' || *p == ':') ++sentence_marks;
    return sentence_marks == 0;
}

static int evidence_supports(const char *candidate, const char *evidence)
{
    static const char *risky[] = {
        "does not require authentication", "does not require authorization",
        "safest of all", "always", "never", "plain text"
    };
    size_t i;
    if (candidate == NULL) return 0;
    for (i = 0; i < sizeof(risky) / sizeof(risky[0]); ++i)
        if (contains_ci(candidate, risky[i]) && !contains_ci(evidence, risky[i])) return 0;
    return 1;
}

static int contradictory_relationship(const char *candidate, const char *evidence)
{
    static const char *claims[] = {
        "controller receives input from the model", "controllers receive input from the model",
        "controller gets input from the model", "controllers get input from the model",
        "view sends data to the controller", "views send data to the controller",
        "model receives the request", "models receive the request"
    };
    size_t i;
    if (candidate == NULL) return 0;
    for (i = 0; i < sizeof(claims) / sizeof(claims[0]); ++i)
        if (contains_ci(candidate, claims[i]) && !contains_ci(evidence, claims[i])) return 1;
    return 0;
}

static int unsupported_primary_responsibility(const char *candidate, const char *evidence)
{
    static const char *prefixes[] = {"main responsibility","primary responsibility","main purpose","primary purpose","responsible for"};
    static const char *narrow_actions[] = {"creating new objects","updating existing ones","database","insert","delete records","write records"};
    size_t i, j;
    if (candidate == NULL) return 0;
    for (i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        if (!contains_ci(candidate, prefixes[i])) continue;
        for (j = 0; j < sizeof(narrow_actions) / sizeof(narrow_actions[0]); ++j)
            if (contains_ci(candidate, narrow_actions[j]) && !contains_ci(evidence, prefixes[i])) return 1;
    }
    return 0;
}

static int evidence_divergence(const char *question, const char *candidate, const char *evidence)
{
    char qt[VALIDATOR_TERM_COUNT][VALIDATOR_TERM_MAX];
    char ct[VALIDATOR_TERM_COUNT][VALIDATOR_TERM_MAX];
    char et[VALIDATOR_TERM_COUNT][VALIDATOR_TERM_MAX];
    size_t qn, cn, en, i;
    unsigned int relevant_evidence_terms = 0;
    unsigned int answer_terms = 0;
    unsigned int supported_answer_terms = 0;

    if (question == NULL || candidate == NULL || evidence == NULL || evidence[0] == '\0') return 0;
    memset(qt, 0, sizeof(qt));
    memset(ct, 0, sizeof(ct));
    memset(et, 0, sizeof(et));
    qn = collect_terms(question, qt);
    cn = collect_terms(candidate, ct);
    en = collect_terms(evidence, et);
    if (qn == 0 || cn == 0 || en == 0) return 0;

    /* Only enforce grounding when the supplied evidence is actually about the question. */
    for (i = 0; i < qn; ++i)
        if (has_term(et, en, qt[i])) ++relevant_evidence_terms;
    /* [AI:GPT-6 | 2026-10-09] Unrelated evidence cannot support a
     * substantive answer. Previously this condition passed silently. */
    if (relevant_evidence_terms == 0) return 1;

    /*
     * Question vocabulary proves only that the candidate is on-topic. It must never
     * be counted as factual support. Evaluate only answer-bearing terms: candidate
     * terms that were not already supplied by the operator's question.
     */
    for (i = 0; i < cn; ++i) {
        if (has_term(qt, qn, ct[i])) continue;
        ++answer_terms;
        if (has_term(et, en, ct[i])) ++supported_answer_terms;
    }

    if (answer_terms == 0) return 1;

    /*
     * A factual answer may paraphrase, so exact lexical identity is not required.
     * It must, however, carry substantive answer content from retained evidence.
     * Requiring at least half of answer-bearing terms to be evidenced rejects a
     * fluent invented predicate while allowing modest connective/paraphrase terms.
     */
    if (supported_answer_terms == 0) return 1;
    if (answer_terms >= 2 && supported_answer_terms * 2U < answer_terms) return 1;
    return 0;
}

/* [AI:GPT-6 | 2026-10-10] A definition answer needs a subject-to-
 * definition relation in evidence. Merely mentioning the subject is not
 * evidence that the sentence defines it. Conservative: reject if unclear. */
/* [AI:GPT-6 | 2026-10-10] Definition claims require an attested
 * subject/predicate relation, not a vocabulary-only match. */
static int bounded_word_ci(const char *begin,size_t length,const char *word)
{
 size_t i;
 if(!begin||!word)return 0;
 for(i=0;i<length;++i)
  if(tolower((unsigned char)begin[i])!=tolower((unsigned char)word[i]))return 0;
 return 1;
}
static int unsupported_definition(const char *question,const char *evidence)
{
 const char *p=question,*subject,*e;
 size_t n;
 if(!question||!evidence)return 0;
 while(isspace((unsigned char)*p))++p;
 if(strncasecmp(p,"what does ",10)!=0)return 0;
 p+=10;subject=p;
 while(isalnum((unsigned char)*p)||*p=='_'||*p=='-')++p;
 n=(size_t)(p-subject);
 if(!n||!isspace((unsigned char)*p))return 0;
 while(isspace((unsigned char)*p))++p;
 if(strncasecmp(p,"mean",4)!=0)return 0;
 p+=4;while(isspace((unsigned char)*p))++p;
 if(*p!='?'&&*p!='.'&&*p!='\0')return 0;
 if(*p&&p[1])return 0;
 for(e=evidence;*e;){
  const char *word,*after;
  size_t len;
  while(*e&&!isalnum((unsigned char)*e)&&*e!='_'&&*e!='-')++e;
  word=e;
  while(*e&&(isalnum((unsigned char)*e)||*e=='_'||*e=='-'))++e;
  len=(size_t)(e-word);
  if(!len)break;
  if(len!=n||!bounded_word_ci(word,len,subject))continue;
  after=e;
  while(isspace((unsigned char)*after))++after;
  if(*after=='='||*after==',') {
   /* A comma-separated alias list is not a predicate on its own. */
   if(*after=='=')return 0;
   if(*after==','){
    const char *eq=strchr(after,'=');
    if(eq&&!(memchr(after,'\n',(size_t)(eq-after))))return 0;
   }
  }
  if(strncasecmp(after,"means ",6)==0||strncasecmp(after,"is ",3)==0)
   return 0;
 }
 return 1;
}

/* [AI:GPT-6 | 2026-10-10] Verbatim relationship support for
 * definition answers. A candidate's predicate must match the attested
 * meaning, not merely repeat the question's subject. */
static int word_present_ci(const char *text,const char *word,size_t length)
{
 const char *p=text,*start;
 if(!text||!word||!length)return 0;
 while(*p){
  while(*p&&!isalnum((unsigned char)*p)&&*p!='_'&&*p!='-')++p;
  start=p;
  while(*p&&(isalnum((unsigned char)*p)||*p=='_'||*p=='-'))++p;
  if((size_t)(p-start)==length&&bounded_word_ci(start,length,word))return 1;
 }
 return 0;
}
static int is_definition_form(const char *question)
{
 const char *p=question;
 if(!p)return 0;
 while(isspace((unsigned char)*p))++p;
 return strncasecmp(p,"what does ",10)==0&&contains_ci(p," mean");
}
static int definition_answer_diverges(const char *question,const char *candidate,
                                     const char *evidence)
{
 const char *p=question,*subject,*rhs=NULL,*e;
 size_t subject_length,meaning_length;
 if(!is_definition_form(question))return 0;
 if(!candidate||!evidence)return 1;
 while(isspace((unsigned char)*p))++p;
 p+=10;subject=p;
 while(isalnum((unsigned char)*p)||*p=='_'||*p=='-')++p;
 subject_length=(size_t)(p-subject);
 if(!subject_length||!word_present_ci(candidate,subject,subject_length))return 1;
 /* The first attested authorized subject relation determines the
  * predicate; ambiguous/multiple relations are rejected upstream. */
 for(e=evidence;*e;){
  const char *start,*after,*eq;
  size_t n;
  while(*e&&!isalnum((unsigned char)*e)&&*e!='_'&&*e!='-')++e;
  start=e;
  while(*e&&(isalnum((unsigned char)*e)||*e=='_'||*e=='-'))++e;
  n=(size_t)(e-start);
  if(!n)break;
  if(n!=subject_length||!bounded_word_ci(start,n,subject))continue;
  after=e;
  while(isspace((unsigned char)*after))++after;
  if(*after=='=')rhs=after+1;
  else if(strncasecmp(after,"means ",6)==0)rhs=after+6;
  else if(strncasecmp(after,"is ",3)==0)rhs=after+3;
  else if(*after==','){
   eq=strchr(after,'=');
   if(eq&&!memchr(after,'\n',(size_t)(eq-after)))rhs=eq+1;
  }
  if(rhs)break;
 }
 if(!rhs)return 1;
 while(*rhs&&!(isalnum((unsigned char)*rhs)||*rhs=='_'||*rhs=='-'))++rhs;
 p=rhs;
 while(isalnum((unsigned char)*p)||*p=='_'||*p=='-')++p;
 meaning_length=(size_t)(p-rhs);
 return !meaning_length||!word_present_ci(candidate,rhs,meaning_length);
}

static stnlabz_module_result_t inbound_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *context)
{
    const digit_validator_inbound_request_t *in;
    digit_validator_inbound_result_t *out;
    (void)context;
    if (request == NULL || response == NULL || response_used == NULL || request_size != sizeof(*in) || response_size < sizeof(*out)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    in = (const digit_validator_inbound_request_t *)request;
    out = (digit_validator_inbound_result_t *)response;
    memset(out, 0, sizeof(*out));
    trim_copy(in->raw, out->normalized, sizeof(out->normalized));
    if (out->normalized[0] == '\0') {
        out->status = DIGIT_VALIDATOR_FAIL;
        out->confidence = DIGIT_VALIDATOR_CONFIDENCE_LOW;
        snprintf(out->reason, sizeof(out->reason), "EMPTY_INPUT");
    } else {
        replace_word(out->normalized, sizeof(out->normalized), " numer ", " number ");
        replace_word(out->normalized, sizeof(out->normalized), " teh ", " the ");
        replace_word(out->normalized, sizeof(out->normalized), " you general orders", " your general orders");
        replace_word(out->normalized, sizeof(out->normalized), "GGUG", "GGUF");
        replace_word(out->normalized, sizeof(out->normalized), "ggug", "gguf");
        out->status = DIGIT_VALIDATOR_PASS;
        out->confidence = strcmp(in->raw, out->normalized) == 0 ? DIGIT_VALIDATOR_CONFIDENCE_HIGH : DIGIT_VALIDATOR_CONFIDENCE_MODERATE;
        snprintf(out->reason, sizeof(out->reason), "PASS");
    }
    *response_used = sizeof(*out);
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t outbound_service(const void *request, size_t request_size, void *response, size_t response_size, size_t *response_used, void *context)
{
    const digit_validator_outbound_request_t *in;
    digit_validator_outbound_result_t *out;
    const char *question;
    (void)context;
    if (request == NULL || response == NULL || response_used == NULL || request_size != sizeof(*in) || response_size < sizeof(*out)) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    in = (const digit_validator_outbound_request_t *)request;
    out = (digit_validator_outbound_result_t *)response;
    memset(out, 0, sizeof(*out));
    question = in->normalized[0] ? in->normalized : in->raw;
    out->status = DIGIT_VALIDATOR_FAIL;
    out->retry_allowed = in->attempt < DIGIT_VALIDATOR_MAX_ATTEMPTS;

    if (in->candidate[0] == '\0') snprintf(out->reason, sizeof(out->reason), "EMPTY_RESPONSE");
    else if (unsupported_definition(question, in->evidence)) snprintf(out->reason, sizeof(out->reason), "DEFINITION_RELATION_MISSING");
    else if (definition_answer_diverges(question,in->candidate,in->evidence)) snprintf(out->reason,sizeof(out->reason),"DEFINITION_ANSWER_DIVERGENCE");
    else if (nearly_echo(question, in->candidate)) snprintf(out->reason, sizeof(out->reason), "ECHO");
    else if (asks_explain(question) && weak_explanation(in->candidate)) snprintf(out->reason, sizeof(out->reason), "OPERATION_INCOMPLETE");
    else if (contradictory_relationship(in->candidate, in->evidence)) snprintf(out->reason, sizeof(out->reason), "CONTRADICTORY_RELATIONSHIP");
    else if (unsupported_primary_responsibility(in->candidate, in->evidence)) snprintf(out->reason, sizeof(out->reason), "OVERBROAD_CLAIM");
    else if (!is_definition_form(question) && evidence_divergence(question, in->candidate, in->evidence)) snprintf(out->reason, sizeof(out->reason), "EVIDENCE_DIVERGENCE");
    else if (!evidence_supports(in->candidate, in->evidence)) snprintf(out->reason, sizeof(out->reason), "UNSUPPORTED_CLAIM");
    else {
        out->status = DIGIT_VALIDATOR_PASS;
        out->retry_allowed = 0;
        snprintf(out->reason, sizeof(out->reason), "PASS");
    }
    *response_used = sizeof(*out);
    return STNLABZ_MODULE_OK;
}

/* [AI:GPT-6 | 2026-10-09] Execute deterministic qualification;
 * a declaration of ten successes is not a qualification test. */
static stnlabz_module_result_t validator_qualify(stnlabz_module_qualification_result_t *result)
{
    static const struct {const char *question,*candidate,*evidence;int reject;} cases[] = {
        {"What is the first General Order?","Remain at the assigned mission.","The first General Order is to remain at the assigned mission.",0},
        {"What is the first General Order?","The database is offline.","The first General Order is to remain at the assigned mission.",1},
        {"What is your mission?","The database is offline.","A bridge network was configured.",1},
        {"Compare C and Python","C is compiled and Python is interpreted.","C is compiled. Python is interpreted.",0},
        {"Compare C and Python","The server is operational.","C is compiled. Python is interpreted.",1},
        {"What is your mission?","What is your mission?","Digit's mission is controlled engineering.",1},
        {"Is authentication needed?","Authentication is never needed.","Authentication is required.",1},
        {"What is the service state?","The service is active.","The service is active.",0},
        {"What is the service state?","","The service is active.",1},
        {"What is the service state?","The service is broken.","The service is active.",1},
        {"What does widget mean?","An object has a type.","An object does not mean it has no type.",1},
        {"What does widget mean?","A widget is a component.","widget means component",0},
        {"What does widget mean?","A widget is a component.","widget, widget, = component, component",0},
        {"What does widget mean?","A widget is a gadget.","widget means component",1}
    };
    size_t i;unsigned int passed=0;
    if(result==NULL)return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    memset(result,0,sizeof(*result));
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        digit_validator_outbound_request_t request;
        digit_validator_outbound_result_t response;
        size_t used=0;
        memset(&request,0,sizeof(request));
        snprintf(request.normalized,sizeof(request.normalized),"%s",cases[i].question);
        snprintf(request.candidate,sizeof(request.candidate),"%s",cases[i].candidate);
        snprintf(request.evidence,sizeof(request.evidence),"%s",cases[i].evidence);
        request.attempt=1;
        if(outbound_service(&request,sizeof(request),&response,sizeof(response),&used,NULL)==STNLABZ_MODULE_OK &&
           used==sizeof(response) &&
           (response.status==DIGIT_VALIDATOR_FAIL)==cases[i].reject)passed++;
    }
    result->tests_executed=(unsigned int)(sizeof(cases)/sizeof(cases[0]));
    result->tests_passed=passed;
    result->tests_failed=result->tests_executed-passed;
    result->negative_test_executed=1;
    result->negative_test_passed=evidence_divergence("What is your mission?","A database is online.","Completely unrelated records.");
    return result->tests_failed||!result->negative_test_passed?
      STNLABZ_MODULE_ERR_QUALIFICATION:STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t validator_start(const stnlabz_module_host_t *host)
{
    if (host == NULL || host->register_service == NULL || host->unregister_service == NULL) return STNLABZ_MODULE_ERR_INVALID_ARGUMENT;
    if (!host->register_service(DIGIT_VALIDATOR_INBOUND_SERVICE, inbound_service, NULL)) return STNLABZ_MODULE_ERR_START_FAILED;
    if (!host->register_service(DIGIT_VALIDATOR_OUTBOUND_SERVICE, outbound_service, NULL)) {
        host->unregister_service(DIGIT_VALIDATOR_INBOUND_SERVICE, NULL);
        return STNLABZ_MODULE_ERR_START_FAILED;
    }
    validator_host = host;
    if (host->send_message != NULL) host->send_message("Validator active.");
    return STNLABZ_MODULE_OK;
}

static stnlabz_module_result_t validator_stop(void)
{
    int ok = 1;
    if (validator_host != NULL && validator_host->unregister_service != NULL) {
        if (!validator_host->unregister_service(DIGIT_VALIDATOR_OUTBOUND_SERVICE, NULL)) ok = 0;
        if (!validator_host->unregister_service(DIGIT_VALIDATOR_INBOUND_SERVICE, NULL)) ok = 0;
    }
    validator_host = NULL;
    return ok ? STNLABZ_MODULE_OK : STNLABZ_MODULE_ERR_STOP_FAILED;
}

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void)
{
    static const stnlabz_module_descriptor_t descriptor = {
        "validator", "Validator", 1, 0, 8,
        STNLABZ_MODULE_API_MAJOR, STNLABZ_MODULE_API_MINOR,
        validator_qualify, validator_start, validator_stop
    };
    return &descriptor;
}
