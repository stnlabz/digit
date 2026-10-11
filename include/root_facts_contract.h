#ifndef DIGIT_ROOT_FACTS_CONTRACT_H
#define DIGIT_ROOT_FACTS_CONTRACT_H
#define DIGIT_ROOT_FACTS_SERVICE "root.facts.list"
#define DIGIT_ROOT_FACTS_MAX 16U
#define DIGIT_ROOT_FACT_TEXT_MAX 128U
typedef struct {
 char subject[DIGIT_ROOT_FACT_TEXT_MAX];
 char predicate[DIGIT_ROOT_FACT_TEXT_MAX];
 char object[DIGIT_ROOT_FACT_TEXT_MAX];
} digit_root_fact_t;
typedef struct {
 unsigned int count;
 digit_root_fact_t facts[DIGIT_ROOT_FACTS_MAX];
} digit_root_facts_result_t;
#endif
