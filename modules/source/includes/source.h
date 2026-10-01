#ifndef DIGIT_SOURCE_H
#define DIGIT_SOURCE_H

#include <stddef.h>
#include "module.h"

#define DIGIT_SOURCE_ROOT "/opt/digit/sources"
#define DIGIT_SOURCE_PROJECT_MAX 64
#define DIGIT_SOURCE_PATH_MAX 512
#define DIGIT_SOURCE_QUERY_MAX 256
#define DIGIT_SOURCE_TEXT_MAX 4096
#define DIGIT_SOURCE_PROJECTS_MAX 64
#define DIGIT_SOURCE_SEARCH_MAX 16

#define DIGIT_SOURCE_PROJECTS_SERVICE "source.projects"
#define DIGIT_SOURCE_SEARCH_SERVICE "source.search"
#define DIGIT_SOURCE_READ_SERVICE "source.read"
#define DIGIT_SOURCE_STATUS_SERVICE "source.status"

typedef struct
{
    char name[DIGIT_SOURCE_PROJECT_MAX];
} digit_source_project_t;

typedef struct
{
    size_t count;
    digit_source_project_t projects[DIGIT_SOURCE_PROJECTS_MAX];
} digit_source_projects_result_t;

typedef struct
{
    char project[DIGIT_SOURCE_PROJECT_MAX];
    char query[DIGIT_SOURCE_QUERY_MAX];
} digit_source_search_request_t;

typedef struct
{
    char project[DIGIT_SOURCE_PROJECT_MAX];
    char path[DIGIT_SOURCE_PATH_MAX];
    size_t line;
    char text[DIGIT_SOURCE_TEXT_MAX];
} digit_source_match_t;

typedef struct
{
    size_t count;
    digit_source_match_t matches[DIGIT_SOURCE_SEARCH_MAX];
} digit_source_search_result_t;

typedef struct
{
    char project[DIGIT_SOURCE_PROJECT_MAX];
    char path[DIGIT_SOURCE_PATH_MAX];
    size_t start_line;
    size_t line_count;
} digit_source_read_request_t;

typedef struct
{
    int found;
    char project[DIGIT_SOURCE_PROJECT_MAX];
    char path[DIGIT_SOURCE_PATH_MAX];
    size_t start_line;
    size_t end_line;
    char text[DIGIT_SOURCE_TEXT_MAX];
} digit_source_read_result_t;

typedef struct
{
    char project[DIGIT_SOURCE_PROJECT_MAX];
} digit_source_status_request_t;

typedef struct
{
    int found;
    int git_repository;
    char project[DIGIT_SOURCE_PROJECT_MAX];
    char head[65];
} digit_source_status_result_t;

const stnlabz_module_descriptor_t *stnlabz_module_get_descriptor(void);

#endif
