/*
 * fmd5 명령어 구현 파일.
 * BFS로 디렉토리를 탐색하고, 후보 파일을 링크드 리스트에 저장한 뒤 MD5 해시로 중복 세트를 만든다.
 */

#include <sys/wait.h>
#include <sys/time.h>
#include <dirent.h>
#include <pwd.h>
#include <strings.h>
#include <limits.h>
#include <utime.h>

#if defined(__has_include)
#  if __has_include(<openssl/evp.h>)
#    include <openssl/evp.h>
#  else
#    define SSU_OPENSSL_EVP_FALLBACK 1
#  endif
#else
#  include <openssl/evp.h>
#endif

#ifdef SSU_OPENSSL_EVP_FALLBACK
/*
 * OpenSSL 헤더가 없는 로컬 환경에서도 libcrypto 런타임을 호출할 수 있게 최소 EVP 선언을 둔다.
 * 채점 환경에 헤더가 있으면 위의 정식 헤더가 사용된다.
 */
#define EVP_MAX_MD_SIZE 64
typedef struct evp_md_ctx_st EVP_MD_CTX;
typedef struct evp_md_st EVP_MD;
EVP_MD_CTX *EVP_MD_CTX_new(void);
void EVP_MD_CTX_free(EVP_MD_CTX *ctx);
const EVP_MD *EVP_md5(void);
int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, void *impl);
int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *data, size_t count);
int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s);
#endif

#include "common.h"

#define MAX_EXCLUDE_DIRS 3      /* -e 옵션은 최대 3개 디렉토리까지 받는다. */

/* 탐색된 정규 파일 하나의 경로와 stat 정보를 보관하는 링크드 리스트 노드다. */
typedef struct FileNode {
    char    path[SSU_PATH_MAX];
    off_t   size;
    time_t  mtime;
    time_t  atime;
    time_t  ctime_;             /* ctime() 함수 이름과 겹치지 않게 밑줄을 붙인다. */
    ino_t   inode;
    dev_t   device;
    mode_t  mode;
    struct FileNode *next;
} FileNode;

/* 같은 크기와 같은 해시를 가진 파일들을 하나의 중복 세트로 묶는다. */
typedef struct FileSet {
    off_t    size;
    char     hash[MD5_HEX_LEN];
    int      count;
    FileNode *head;
    struct FileSet *next;
} FileSet;

/* 출력 대상이 되는 전체 중복 세트 목록이다. */
typedef struct SetList {
    int      count;
    FileSet *head;
} SetList;

/* f/t 삭제 동작에서 어떤 파일을 남길지 결정하는 추가 옵션 -u의 기준이다. */
typedef enum {
    PRESERVE_NEWEST,
    PRESERVE_OLDEST,
    PRESERVE_PATHSHORT,
    PRESERVE_PATHLONG
} PreserveRule;

/* 탐색 결과 세트 정렬 기준을 고르는 추가 옵션 -r의 기준이다. */
typedef enum {
    SORT_DEFAULT,
    SORT_SIZE_DESC,
    SORT_MTIME_DESC,
    SORT_PATH_ASC
} SortRule;

/* 명령행 옵션을 해석한 결과를 한 구조체에 모아 BFS 필터에서 사용한다. */
typedef struct FilterContext {
    /* 확장자와 크기 범위 조건이다. */
    int   has_size_filter;
    char  ext_pattern[SSU_NAME_MAX];
    int   ext_match_all;
    int   min_unlimited, max_unlimited;
    off_t min_size, max_size;

    /* 접근/수정/속성변경 시간 범위 조건이다. */
    int    has_atime;
    time_t atime_from, atime_to;
    int    has_mtime;
    time_t mtime_from, mtime_to;
    int    has_ctime;
    time_t ctime_from, ctime_to;

    /* 하드링크 제외 여부다. */
    int exclude_hardlinks;

    /* 파일 권한 필터다. */
    int    has_mode_filter;
    mode_t mode_filter;

    /* 탐색에서 제외할 디렉토리 목록이다. */
    int  exclude_count;
    char exclude_dirs[MAX_EXCLUDE_DIRS][SSU_PATH_MAX];

    /* BFS 깊이 제한과 출력 세트 개수 제한이다. */
    int max_depth;
    int max_count;

    /* 추가 옵션 -r/-o/-x의 실행 기준이다. */
    SortRule sort_rule;
    int      min_dup_count;
    int      has_output_path;
    char     output_path[SSU_PATH_MAX];

    int opt_d_seen;
    int opt_n_seen;
    int opt_e_seen;
    int opt_p_seen;
    int opt_r_seen;
    int opt_o_seen;
    int opt_x_seen;
} FilterContext;

/* 크기/시간 인자의 파싱 결과를 구분한다. */
typedef enum {
    PARSE_OK,
    PARSE_UNLIMITED,            /* "~"는 제한 없음으로 해석한다. */
    PARSE_ERROR
} ParseResult;

/* BFS 큐에 들어가는 디렉토리 경로와 깊이 정보다. */
typedef struct DirNode {
    char path[SSU_PATH_MAX];
    int  depth;
    struct DirNode *next;
} DirNode;

typedef struct DirQueue {
    DirNode *front;
    DirNode *rear;
} DirQueue;

/* FileNode를 생성해 경로와 stat 정보를 복사한다. */
static FileNode *filenode_create(const char *path, const struct stat *st)
{
    /* 잘못된 호출은 후보 목록에 넣을 수 없으므로 NULL로 거부한다. */
    if (path == NULL || st == NULL) return NULL;

    /* 파일 하나를 표현할 노드를 동적 할당한다. */
    FileNode *node = (FileNode *)malloc(sizeof(FileNode));
    if (node == NULL) return NULL;

    /* 이후 출력과 필터링에 필요한 stat 필드를 모두 노드에 복사한다. */
    snprintf(node->path, sizeof(node->path), "%s", path);
    node->size   = st->st_size;
    node->mtime  = st->st_mtime;
    node->atime  = st->st_atime;
    node->ctime_ = st->st_ctime;
    node->inode  = st->st_ino;
    node->device = st->st_dev;
    node->mode   = st->st_mode;
    node->next   = NULL;
    return node;
}

/* FileNode 목록 전체를 해제한다. */
static void filenode_list_free(FileNode *head)
{
    /* next를 먼저 저장해 둔 뒤 현재 노드를 해제해야 다음 노드로 이동할 수 있다. */
    while (head != NULL) {
        FileNode *next = head->next;
        free(head);
        head = next;
    }
}

/* 후보 파일 목록의 뒤쪽에 새 노드를 붙여 BFS 발견 순서를 유지한다. */
static void filenode_list_append(FileNode **head, FileNode **tail, FileNode *node)
{
    /* head/tail 포인터 자체가 없거나 붙일 노드가 없으면 작업하지 않는다. */
    if (head == NULL || tail == NULL || node == NULL) return;

    /* 새 노드는 항상 목록의 마지막 노드가 되므로 next를 비운다. */
    node->next = NULL;

    /* 빈 목록이면 head와 tail이 모두 새 노드를 가리키게 한다. */
    if (*head == NULL) {
        *head = node;
        *tail = node;
    } else {
        /* 기존 tail 뒤에 붙이고 tail 포인터를 새 노드로 옮긴다. */
        (*tail)->next = node;
        *tail = node;
    }
}

/* 세트 내부 파일은 절대경로 길이, ASCII 순서로 정렬한다. */
static int filenode_cmp(const FileNode *a, const FileNode *b)
{
    /* 먼저 경로 길이가 짧은 파일을 앞에 두어 명세 출력 순서를 맞춘다. */
    size_t la = strlen(a->path);
    size_t lb = strlen(b->path);
    if (la != lb) return (la < lb) ? -1 : 1;

    /* 길이가 같으면 문자열 사전순으로 안정적인 출력 순서를 만든다. */
    return strcmp(a->path, b->path);
}

/* 링크드 리스트를 배열 없이 삽입 정렬한다. */
static void filenode_list_sort(FileNode **head)
{
    /* 노드가 0개 또는 1개면 이미 정렬된 상태다. */
    if (head == NULL || *head == NULL || (*head)->next == NULL) return;

    /* 원본 목록을 하나씩 떼어 sorted 목록의 알맞은 위치에 삽입한다. */
    FileNode *sorted = NULL;
    FileNode *cur = *head;
    while (cur != NULL) {
        FileNode *next = cur->next;

        /* sorted의 맨 앞보다 작거나 같으면 맨 앞에 삽입한다. */
        if (sorted == NULL || filenode_cmp(cur, sorted) <= 0) {
            cur->next = sorted;
            sorted = cur;
        } else {
            /* 아니면 들어갈 위치의 직전 노드를 찾는다. */
            FileNode *p = sorted;
            while (p->next != NULL && filenode_cmp(cur, p->next) > 0) p = p->next;
            cur->next = p->next;
            p->next = cur;
        }
        cur = next;
    }
    /* 정렬이 끝난 새 head를 호출자에게 돌려준다. */
    *head = sorted;
}

/* 1부터 시작하는 인덱스로 세트 내부 파일을 찾는다. */
static FileNode *filenode_list_at(FileNode *head, int index)
{
    /* 삭제 명령의 LIST_IDX는 1부터 시작하므로 0 이하는 무효다. */
    if (index < 1) return NULL;

    /* 순차 탐색으로 index번째 노드를 찾는다. */
    int i = 1;
    for (FileNode *p = head; p != NULL; p = p->next, i++) {
        if (i == index) return p;
    }
    return NULL;
}

/* 1부터 시작하는 인덱스의 파일 노드를 목록에서 제거한다. */
static int filenode_list_remove_at(FileNode **head, int index)
{
    /* 빈 목록이거나 잘못된 인덱스면 제거하지 않는다. */
    if (head == NULL || *head == NULL || index < 1) return 0;

    /* 첫 번째 노드는 head 자체를 다음 노드로 옮긴 뒤 해제한다. */
    if (index == 1) {
        FileNode *to_free = *head;
        *head = to_free->next;
        free(to_free);
        return 1;
    }

    /* 두 번째 이후 노드는 제거 대상의 직전 노드를 찾아 연결을 건너뛴다. */
    FileNode *prev = *head;
    int i = 2;
    while (prev->next != NULL && i < index) { prev = prev->next; i++; }
    if (prev->next == NULL) return 0;
    FileNode *to_free = prev->next;
    prev->next = to_free->next;
    free(to_free);
    return 1;
}

/* 새 중복 세트를 만든다. */
static FileSet *fileset_create(off_t size, const char *hash)
{
    /* 같은 크기/해시 그룹 하나를 표현할 세트를 동적 할당한다. */
    FileSet *set = (FileSet *)malloc(sizeof(FileSet));
    if (set == NULL) return NULL;

    /* by_size 임시 그룹은 해시가 없을 수 있으므로 빈 문자열도 허용한다. */
    set->size = size;
    if (hash != NULL) snprintf(set->hash, sizeof(set->hash), "%s", hash);
    else              set->hash[0] = '\0';
    set->count = 0;
    set->head  = NULL;
    set->next  = NULL;
    return set;
}

/* 세트에 파일 노드를 붙이고 세트 크기를 갱신한다. */
static void fileset_add(FileSet *set, FileNode *node)
{
    /* 세트나 노드가 없으면 목록을 수정하지 않는다. */
    if (set == NULL || node == NULL) return;

    /* 앞에 붙여 O(1)로 추가하고, 이후 별도 정렬로 출력 순서를 맞춘다. */
    node->next = set->head;
    set->head = node;
    set->count++;
}

/* 세트와 세트에 포함된 파일 목록을 함께 해제한다. */
static void fileset_free(FileSet *set)
{
    /* NULL 세트는 이미 비어 있는 것으로 보고 넘어간다. */
    if (set == NULL) return;

    /* 세트가 소유한 파일 노드들을 먼저 해제한 뒤 세트 자체를 해제한다. */
    filenode_list_free(set->head);
    free(set);
}

/* 세트 대표 경로 비교를 위해 세트 안에서 가장 짧은 경로 길이를 구한다. */
static int fileset_shortest_path_len(const FileSet *set)
{
    /* 비교에 쓸 대표 길이가 없으면 0으로 처리한다. */
    if (set == NULL || set->head == NULL) return 0;

    /* 첫 경로 길이를 기준으로 잡고 나머지를 순회하며 최소값을 갱신한다. */
    int min_len = (int)strlen(set->head->path);
    for (FileNode *p = set->head->next; p != NULL; p = p->next) {
        int l = (int)strlen(p->path);
        if (l < min_len) min_len = l;
    }
    return min_len;
}

/* f/t 삭제 옵션에서 남길 가장 최근 수정 파일을 고른다. */
static FileNode *fileset_newest_mtime(const FileSet *set)
{
    /* 삭제 대상 세트가 없으면 남길 파일도 없다. */
    if (set == NULL || set->head == NULL) return NULL;

    /* 첫 파일을 임시 기준으로 두고 더 최근 mtime을 가진 파일을 찾는다. */
    FileNode *best = set->head;
    for (FileNode *p = set->head->next; p != NULL; p = p->next) {
        if (p->mtime > best->mtime) best = p;
    }
    return best;
}

/* 세트 안에서 가장 늦은 mtime을 찾아 -r mtime_desc 정렬 기준으로 사용한다. */
static time_t fileset_latest_mtime(const FileSet *set)
{
    /* 비어 있는 세트는 가장 오래된 값처럼 취급한다. */
    if (set == NULL || set->head == NULL) return 0;

    /* 첫 파일 시간을 기준으로 두고 더 늦은 시간을 찾는다. */
    time_t latest = set->head->mtime;
    for (FileNode *p = set->head->next; p != NULL; p = p->next) {
        if (p->mtime > latest) latest = p->mtime;
    }
    return latest;
}

/* 세트 안에서 사전순으로 가장 앞선 절대경로를 찾아 -r path_asc 기준으로 사용한다. */
static const char *fileset_min_path_lex(const FileSet *set)
{
    /* 비어 있는 세트는 비교용 빈 문자열을 돌려준다. */
    if (set == NULL || set->head == NULL) return "";

    /* 모든 경로를 비교해 사전순 대표 경로를 고른다. */
    const char *best = set->head->path;
    for (FileNode *p = set->head->next; p != NULL; p = p->next) {
        if (strcmp(p->path, best) < 0) best = p->path;
    }
    return best;
}

/* -u 규칙에 따라 f/t에서 남길 파일 하나를 선택한다. */
static FileNode *fileset_select_preserve(const FileSet *set, PreserveRule rule)
{
    /* 세트가 비었으면 선택할 파일이 없다. */
    if (set == NULL || set->head == NULL) return NULL;

    /* 기본 newest는 기존 함수로 처리해 원래 f/t 동작과 같은 기준을 유지한다. */
    if (rule == PRESERVE_NEWEST) return fileset_newest_mtime(set);

    /* 첫 파일을 임시 보존 대상으로 잡고 규칙별로 더 적합한 파일을 찾는다. */
    FileNode *best = set->head;
    for (FileNode *p = set->head->next; p != NULL; p = p->next) {
        size_t best_len = strlen(best->path);
        size_t cur_len = strlen(p->path);

        if (rule == PRESERVE_NEWEST) {
            if (p->mtime > best->mtime) best = p;
        } else if (rule == PRESERVE_OLDEST) {
            if (p->mtime < best->mtime) best = p;
        } else if (rule == PRESERVE_PATHSHORT) {
            if (cur_len < best_len || (cur_len == best_len && strcmp(p->path, best->path) < 0))
                best = p;
        } else if (rule == PRESERVE_PATHLONG) {
            if (cur_len > best_len || (cur_len == best_len && strcmp(p->path, best->path) < 0))
                best = p;
        }
    }
    return best;
}

/* 세트 목록을 빈 상태로 초기화한다. */
static void setlist_init(SetList *list)
{
    /* 호출자가 넘긴 목록 구조체를 빈 링크드 리스트 상태로 만든다. */
    if (list == NULL) return;
    list->head = NULL;
    list->count = 0;
}

/* 세트 목록에 새 세트를 붙인다. 실제 출력 순서는 이후 정렬에서 정한다. */
static void setlist_append(SetList *list, FileSet *set)
{
    /* 잘못된 포인터가 들어오면 기존 목록을 유지한다. */
    if (list == NULL || set == NULL) return;

    /* 앞에 붙인 뒤 별도 정렬을 수행해 링크드 리스트만으로 순서를 맞춘다. */
    set->next = list->head;
    list->head = set;
    list->count++;
}

/* 세트 목록에서 특정 세트를 제거하고 메모리를 해제한다. */
static void setlist_remove(SetList *list, FileSet *target)
{
    /* 제거 대상이 없거나 목록이 비어 있으면 아무 작업도 하지 않는다. */
    if (list == NULL || target == NULL || list->head == NULL) return;

    /* target이 첫 세트면 head만 다음 세트로 바꾸면 된다. */
    if (list->head == target) {
        list->head = target->next;
        fileset_free(target);
        list->count--;
        return;
    }

    /* 첫 세트가 아니면 target 직전 세트를 찾아 연결을 다시 이어 준다. */
    FileSet *prev = list->head;
    while (prev->next != NULL && prev->next != target) prev = prev->next;
    if (prev->next == target) {
        prev->next = target->next;
        fileset_free(target);
        list->count--;
    }
}

/* 전체 세트 목록을 해제한다. */
static void setlist_free(SetList *list)
{
    /* 목록 포인터가 없으면 해제할 것도 없다. */
    if (list == NULL) return;

    /* 모든 세트를 하나씩 떼어 파일 노드까지 함께 해제한다. */
    FileSet *cur = list->head;
    while (cur != NULL) {
        FileSet *next = cur->next;
        fileset_free(cur);
        cur = next;
    }
    list->head = NULL;
    list->count = 0;
}

/* 기본 출력 순서는 크기 오름차순, 최단 경로 길이, 해시 사전순이다. */
static int fileset_cmp(const FileSet *a, const FileSet *b)
{
    /* 명세 기본 출력은 작은 파일 크기 세트부터 보여준다. */
    if (a->size != b->size) return (a->size < b->size) ? -1 : 1;

    /* 크기가 같으면 세트 안의 가장 짧은 경로 길이로 순서를 정한다. */
    int la = fileset_shortest_path_len(a);
    int lb = fileset_shortest_path_len(b);
    if (la != lb) return (la < lb) ? -1 : 1;

    /* 마지막 tie-breaker는 해시 문자열이다. */
    return strcmp(a->hash, b->hash);
}

/* -n 옵션에서는 큰 파일 세트부터 고르기 위해 크기 내림차순으로 비교한다. */
static int fileset_cmp_size_desc(const FileSet *a, const FileSet *b)
{
    /* -n은 큰 중복 세트부터 잘라야 하므로 크기 비교 방향만 뒤집는다. */
    if (a->size != b->size) return (a->size > b->size) ? -1 : 1;

    /* 크기가 같으면 기본 비교 규칙을 그대로 재사용한다. */
    return fileset_cmp(a, b);
}

/* -r mtime_desc는 세트 안에서 가장 최근 수정 시간을 가진 세트를 먼저 출력한다. */
static int fileset_cmp_mtime_desc(const FileSet *a, const FileSet *b)
{
    /* 세트 대표 mtime을 최신값으로 잡고 내림차순 비교한다. */
    time_t ta = fileset_latest_mtime(a);
    time_t tb = fileset_latest_mtime(b);
    if (ta != tb) return (ta > tb) ? -1 : 1;

    /* 시간이 같으면 기본 비교로 출력 순서를 안정화한다. */
    return fileset_cmp(a, b);
}

/* -r path_asc는 세트의 대표 절대경로를 사전순으로 정렬한다. */
static int fileset_cmp_path_asc(const FileSet *a, const FileSet *b)
{
    /* 각 세트에서 가장 앞선 경로를 대표 키로 삼는다. */
    const char *pa = fileset_min_path_lex(a);
    const char *pb = fileset_min_path_lex(b);
    int c = strcmp(pa, pb);
    if (c != 0) return c;

    /* 경로가 같을 수 있는 특수한 경우에는 기본 비교로 마무리한다. */
    return fileset_cmp(a, b);
}

/* 주어진 비교 함수로 세트 목록을 삽입 정렬한다. */
static void setlist_sort_by(SetList *list, int (*cmp)(const FileSet *, const FileSet *))
{
    /* 정렬할 세트가 1개 이하이면 그대로 둔다. */
    if (list == NULL || list->head == NULL || list->head->next == NULL) return;

    /* FileNode 정렬과 같은 방식으로 세트를 하나씩 삽입 정렬한다. */
    FileSet *sorted = NULL;
    FileSet *cur = list->head;
    while (cur != NULL) {
        FileSet *next = cur->next;

        /* 새 세트가 sorted의 맨 앞에 와야 하면 바로 head로 붙인다. */
        if (sorted == NULL || cmp(cur, sorted) <= 0) {
            cur->next = sorted;
            sorted = cur;
        } else {
            /* 아니면 비교 함수 기준으로 들어갈 위치를 찾는다. */
            FileSet *p = sorted;
            while (p->next != NULL && cmp(cur, p->next) > 0) p = p->next;
            cur->next = p->next;
            p->next = cur;
        }
        cur = next;
    }
    /* 정렬된 목록을 원래 SetList에 연결한다. */
    list->head = sorted;
}

/* 기본 명세 순서로 세트 목록을 정렬한다. */
static void setlist_sort(SetList *list)
{
    /* 기본 출력용 비교 함수를 사용한다. */
    setlist_sort_by(list, fileset_cmp);
}

/* -r 옵션에서 고른 정렬 기준을 실제 링크드 리스트 정렬 함수에 연결한다. */
static void setlist_sort_with_rule(SetList *list, SortRule rule)
{
    /* 기본 정렬은 기존 명세의 크기/최단경로/해시 순서를 그대로 사용한다. */
    if (rule == SORT_DEFAULT) {
        setlist_sort(list);
    } else if (rule == SORT_SIZE_DESC) {
        setlist_sort_by(list, fileset_cmp_size_desc);
    } else if (rule == SORT_MTIME_DESC) {
        setlist_sort_by(list, fileset_cmp_mtime_desc);
    } else if (rule == SORT_PATH_ASC) {
        setlist_sort_by(list, fileset_cmp_path_asc);
    }
}

/* 1부터 시작하는 세트 번호로 중복 세트를 찾는다. */
static FileSet *setlist_at(const SetList *list, int index)
{
    /* 삭제 서브 프롬프트의 SET_INDEX도 1부터 시작한다. */
    if (list == NULL || index < 1) return NULL;

    /* 링크드 리스트를 앞에서부터 세어 target 세트를 찾는다. */
    int i = 1;
    for (FileSet *p = list->head; p != NULL; p = p->next, i++) {
        if (i == index) return p;
    }
    return NULL;
}

#define HASH_READ_CHUNK 8192

/* 파일을 바이너리로 읽어 EVP 인터페이스로 MD5 해시 문자열을 계산한다. */
static int md5_compute_file(const char *path, char *hex_out)
{
    /* 경로나 출력 버퍼가 없으면 해시를 저장할 수 없다. */
    if (path == NULL || hex_out == NULL) return 0;

    /* 텍스트 변환 없이 원본 바이트 그대로 읽기 위해 바이너리 모드로 연다. */
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) return 0;

    /* OpenSSL EVP 컨텍스트를 만들고 MD5 알고리즘으로 초기화한다. */
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) { fclose(fp); return 0; }

    if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1) {
        EVP_MD_CTX_free(ctx); fclose(fp); return 0;
    }

    unsigned char buf[HASH_READ_CHUNK];
    size_t n;
    /* 큰 파일도 한 번에 올리지 않고 일정 크기씩 읽어 해시에 누적한다. */
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (EVP_DigestUpdate(ctx, buf, n) != 1) {
            EVP_MD_CTX_free(ctx); fclose(fp); return 0;
        }
    }
    /* 읽기 중 에러가 있으면 잘못된 해시가 나오지 않도록 실패 처리한다. */
    if (ferror(fp)) { EVP_MD_CTX_free(ctx); fclose(fp); return 0; }

    /* 누적된 MD5 결과를 digest 바이트 배열로 확정한다. */
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(ctx); fclose(fp); return 0;
    }
    EVP_MD_CTX_free(ctx);
    fclose(fp);

    /* digest 바이트를 명세 예시와 같은 소문자 16진수 문자열로 바꾼다. */
    static const char hex[] = "0123456789abcdef";
    for (unsigned int i = 0; i < digest_len; i++) {
        hex_out[i * 2]     = hex[(digest[i] >> 4) & 0x0F];
        hex_out[i * 2 + 1] = hex[digest[i] & 0x0F];
    }
    hex_out[digest_len * 2] = '\0';
    return 1;
}

/* 크기 문자열의 숫자 부분을 명세가 허용한 10진 정수/실수 형태로만 자른다. */
static int scan_decimal_number(const char *str, size_t *num_len, int *has_dot)
{
    /* 숫자부는 정수 또는 소수점 한 개가 포함된 실수까지만 허용한다. */
    int dot_seen = 0;
    int digit_count = 0;
    int digit_after_dot = 0;
    size_t i = 0;

    if (str == NULL || str[0] == '\0') return 0;

    /* 단위가 시작되기 전까지 숫자와 점만 소비한다. */
    while (str[i] != '\0') {
        if (isdigit((unsigned char)str[i])) {
            digit_count++;
            if (dot_seen) digit_after_dot++;
            i++;
        } else if (str[i] == '.') {
            /* 두 번째 점은 1.2.3 같은 잘못된 입력이므로 거부한다. */
            if (dot_seen) return 0;
            dot_seen = 1;
            i++;
        } else {
            break;
        }
    }

    /* 숫자가 하나도 없거나 1.처럼 소수점 뒤 숫자가 없으면 에러다. */
    if (digit_count == 0) return 0;
    if (dot_seen && digit_after_dot == 0) return 0;

    /* 호출자는 num_len 뒤의 문자열을 단위로 해석한다. */
    *num_len = i;
    *has_dot = dot_seen;
    return 1;
}

/* 크기 인자를 바이트 수로 바꾼다. 허용 단위는 B, KB, MB, GB와 무제한(~)뿐이다. */
static ParseResult parse_size(const char *str, off_t *out_bytes)
{
    /* NULL 입력은 명세의 크기 형식이 아니므로 거부한다. */
    if (str == NULL || out_bytes == NULL) return PARSE_ERROR;

    /* '~'는 최소/최대 크기 제한 없음으로 따로 표시한다. */
    if (strcmp(str, "~") == 0) return PARSE_UNLIMITED;
    if (str[0] == '\0' || str[0] == '-') return PARSE_ERROR;

    /* 먼저 숫자부만 직접 검증해서 nan, 1e3 같은 입력을 차단한다. */
    size_t num_len = 0;
    int has_dot = 0;
    if (!scan_decimal_number(str, &num_len, &has_dot)) return PARSE_ERROR;
    if (num_len >= 128) return PARSE_ERROR;

    /* strtod에는 검증된 숫자부만 넘겨 C 라이브러리의 확장 표기를 피한다. */
    char number_part[128];
    memcpy(number_part, str, num_len);
    number_part[num_len] = '\0';

    const char *unit = str + num_len;
    char *endptr = NULL;
    errno = 0;
    double num = strtod(number_part, &endptr);
    if (endptr == number_part || *endptr != '\0' || errno == ERANGE || num < 0)
        return PARSE_ERROR;

    /* 남은 문자열을 단위로 해석하고 바이트 배수로 환산한다. */
    double mult = 1.0;
    int    is_byte_unit = 0;
    if (*unit == '\0') {
        mult = 1.0; is_byte_unit = 1;
    } else if (strcasecmp(unit, "kb") == 0) {
        mult = 1024.0;
    } else if (strcasecmp(unit, "mb") == 0) {
        mult = 1024.0 * 1024.0;
    } else if (strcasecmp(unit, "gb") == 0) {
        mult = 1024.0 * 1024.0 * 1024.0;
    } else if (strcasecmp(unit, "b") == 0) {
        mult = 1.0; is_byte_unit = 1;
    } else {
        return PARSE_ERROR;
    }

    /* 바이트 단위는 정수만 허용해 1.5B 같은 애매한 값을 막는다. */
    if (is_byte_unit && has_dot) return PARSE_ERROR;

    /* off_t 범위를 넘는 입력은 오버플로우 전에 거부한다. */
    if (num > (double)LLONG_MAX / mult) return PARSE_ERROR;

    *out_bytes = (off_t)(num * mult);
    return PARSE_OK;
}

/* 윤년 여부를 계산해 날짜 범위 검증에 사용한다. */
static int is_leap_year(int year)
{
    /* 4년 주기이되 100년 예외, 400년 예외를 적용한다. */
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

/* 특정 월의 마지막 일을 구해 0으로 생략된 일자를 보정한다. */
static int days_in_month(int year, int month)
{
    /* 기본 월별 일수를 배열로 두고 범위를 먼저 검증한다. */
    static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month < 1 || month > 12) return 0;

    /* 2월은 윤년일 때만 29일로 보정한다. */
    int d = days[month - 1];
    if (month == 2 && is_leap_year(year)) d = 29;
    return d;
}

/* 시간 인자를 명세 규칙에 따라 시작 시각 또는 종료 시각으로 해석한다. */
static ParseResult parse_time_arg(const char *str, time_t *out, int is_from)
{
    /* '~'는 시간 제한 없음으로 상위 함수에서 실제 경계값으로 바꾼다. */
    if (str == NULL || out == NULL) return PARSE_ERROR;
    if (strcmp(str, "~") == 0) return PARSE_UNLIMITED;

    /* 시간 문자열은 YYYY[:MM[:DD[:HH]]] 형태라 최대 4개 숫자 조각만 허용한다. */
    int vals[4] = {0, 0, 0, 0};
    int parts = 0;
    const char *p = str;
    while (*p != '\0') {
        /* 콜론으로 구분된 각 조각은 비어 있지 않은 양의 10진수여야 한다. */
        if (parts >= 4) return PARSE_ERROR;
        if (!isdigit((unsigned char)*p)) return PARSE_ERROR;

        char *endptr = NULL;
        long v = strtol(p, &endptr, 10);
        if (endptr == p || v < 0 || v > INT_MAX) return PARSE_ERROR;
        vals[parts++] = (int)v;

        if (*endptr == '\0') break;
        if (*endptr != ':') return PARSE_ERROR;
        p = endptr + 1;
        if (*p == '\0') return PARSE_ERROR;
    }
    if (parts < 1) return PARSE_ERROR;

    int year = vals[0];
    int month = (parts >= 2) ? vals[1] : 0;
    int day = (parts >= 3) ? vals[2] : 0;
    int hour = (parts >= 4) ? vals[3] : -1;

    /* 연도는 네 자리 범위로 제한해 mktime의 과도한 보정을 막는다. */
    if (year < 1000 || year > 9999) return PARSE_ERROR;

    /* 0 또는 생략된 월/일/시는 from/to에 따라 시작값 또는 끝값으로 채운다. */
    int month_unspecified = (parts < 2 || month == 0);
    int day_unspecified = (parts < 3 || day == 0);
    int hour_unspecified = (parts < 4);

    if (month_unspecified) {
        /* 월이 생략된 상태에서 일/시만 지정하는 모호한 입력은 거부한다. */
        if ((parts >= 3 && day != 0) || (parts >= 4 && hour != 0)) return PARSE_ERROR;
        month = is_from ? 1 : 12;
        day_unspecified = 1;
        hour_unspecified = 1;
    } else if (month < 1 || month > 12) {
        return PARSE_ERROR;
    }

    int max_day = days_in_month(year, month);
    if (day_unspecified) {
        /* 일자가 없으면 from은 1일, to는 해당 월의 마지막 날로 해석한다. */
        if (parts >= 4 && hour != 0) return PARSE_ERROR;
        day = is_from ? 1 : max_day;
        hour_unspecified = 1;
    } else if (day < 1 || day > max_day) {
        return PARSE_ERROR;
    }

    int min_val, sec_val;
    if (hour_unspecified) {
        /* 시각이 없으면 from은 하루 시작, to는 하루 끝으로 맞춘다. */
        hour    = is_from ? 0 : 23;
        min_val = is_from ? 0 : 59;
        sec_val = is_from ? 0 : 59;
    } else {
        /* 시각이 있으면 분/초는 from과 to 경계에 맞춰 보정한다. */
        if (hour < 0 || hour > 23) return PARSE_ERROR;
        if (is_from) { min_val = 0;  sec_val = 0; }
        else         { min_val = 59; sec_val = 59; }
    }

    /* tm 구조체로 바꾼 뒤 mktime을 통해 time_t 값으로 변환한다. */
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year  = year - 1900;
    t.tm_mon   = month - 1;
    t.tm_mday  = day;
    t.tm_hour  = hour;
    t.tm_min   = min_val;
    t.tm_sec   = sec_val;
    t.tm_isdst = -1;

    time_t result = mktime(&t);
    if (result == (time_t)-1) return PARSE_ERROR;

    /* 최종 시간 경계값을 호출자에게 전달한다. */
    *out = result;
    return PARSE_OK;
}

/* 확장자 인자는 '*' 또는 '*.확장자' 형태만 허용한다. */
static int validate_extension(const char *ext)
{
    /* 빈 확장자 인자는 명세의 FILE_EXTENSION 형식이 아니다. */
    if (ext == NULL || ext[0] == '\0') return 0;

    /* '*'는 모든 확장자를 의미한다. */
    if (strcmp(ext, "*") == 0) return 1;

    /* 특정 확장자는 '*.txt'처럼 반드시 '*.'로 시작해야 한다. */
    if (ext[0] != '*' || ext[1] != '.') return 0;
    if (ext[2] == '\0') return 0;

    /* 경로 구분자나 공백이 섞이면 파일 확장자 패턴으로 보지 않는다. */
    for (const char *p = ext + 2; *p != '\0'; p++) {
        if (*p == '/' || *p == ' ' || *p == '\t') return 0;
    }
    return 1;
}

/* '*.확장자'에서 실제 확장자 문자열만 분리한다. */
static int extract_extension(const char *pattern, char *out_ext, size_t out_size)
{
    /* validate_extension을 통과한 패턴만 실제 비교용 문자열로 바꾼다. */
    if (pattern == NULL) return 0;

    /* '*'는 모든 확장자이므로 비교 문자열을 비워 둔다. */
    if (strcmp(pattern, "*") == 0) {
        if (out_ext != NULL && out_size > 0) out_ext[0] = '\0';
        return 1;
    }

    /* '*.' 다음 부분만 복사해 파일명의 마지막 확장자와 비교한다. */
    if (pattern[0] != '*' || pattern[1] != '.') return 0;
    if (out_ext != NULL && out_size > 0) {
        snprintf(out_ext, out_size, "%s", pattern + 2);
    }
    return 1;
}

/* '~'를 홈 디렉토리로 확장하고 realpath로 절대 디렉토리 경로를 얻는다. */
static int resolve_target_dir(const char *path, char *abs_out, size_t abs_size)
{
    /* 대상 경로와 결과 버퍼가 있어야 실제 탐색 디렉토리를 만들 수 있다. */
    if (path == NULL || path[0] == '\0' || abs_out == NULL) return 0;

    char expanded[SSU_PATH_MAX];
    if (path[0] == '~') {
        /* HOME 환경변수를 우선 사용하고, 없으면 passwd 정보에서 홈을 찾는다. */
        const char *home = getenv("HOME");
        if (home == NULL) {
            struct passwd *pw = getpwuid(getuid());
            if (pw == NULL) return 0;
            home = pw->pw_dir;
        }

        /* "~" 또는 "~/..."만 홈 확장으로 처리하고 "~user" 형식은 그대로 둔다. */
        if (path[1] == '/' || path[1] == '\0') {
            snprintf(expanded, sizeof(expanded), "%s%s", home, path + 1);
        } else {
            snprintf(expanded, sizeof(expanded), "%s", path);
        }
    } else {
        snprintf(expanded, sizeof(expanded), "%s", path);
    }

    /* 상대 경로, 심볼릭 링크, .. 등을 정규화해 절대 경로로 만든다. */
    char resolved[SSU_PATH_MAX];
    if (realpath(expanded, resolved) == NULL) return 0;

    /* 실제 존재하며 디렉토리인 경우만 탐색 시작점으로 허용한다. */
    struct stat st;
    if (stat(resolved, &st) != 0) return 0;
    if (!S_ISDIR(st.st_mode)) return 0;

    /* 호출자 버퍼에 최종 절대 디렉토리 경로를 복사한다. */
    snprintf(abs_out, abs_size, "%s", resolved);
    return 1;
}

/* -o 출력 파일 경로에서 '~'를 홈 디렉토리로 확장한다. */
static int expand_output_path(const char *path, char *out, size_t out_size)
{
    /* 출력 파일 경로와 결과 버퍼가 유효해야 한다. */
    if (path == NULL || path[0] == '\0' || out == NULL || out_size == 0) return 0;

    if (path[0] == '~' && (path[1] == '/' || path[1] == '\0')) {
        /* HOME이 없을 때는 passwd 정보에서 현재 사용자의 홈을 얻는다. */
        const char *home = getenv("HOME");
        if (home == NULL) {
            struct passwd *pw = getpwuid(getuid());
            if (pw == NULL) return 0;
            home = pw->pw_dir;
        }

        /* '~' 뒤의 상대 부분을 홈 경로 뒤에 붙인다. */
        int n = snprintf(out, out_size, "%s%s", home, path + 1);
        return (n >= 0 && (size_t)n < out_size);
    }

    /* '~user' 형식이나 일반 상대/절대 경로는 입력 그대로 사용한다. */
    int n = snprintf(out, out_size, "%s", path);
    return (n >= 0 && (size_t)n < out_size);
}

/* 서브 프롬프트 입력을 공백/탭 기준 argv로 나눈다. */
static int tokenize_line(char *input, char *argv[], int max_tokens)
{
    /* 입력 버퍼와 argv 배열이 유효해야 토큰화를 진행한다. */
    if (input == NULL || argv == NULL || max_tokens < 1) return 0;

    /* 삭제 서브 명령어도 메인 프롬프트처럼 공백/탭 단위로 나눈다. */
    int argc = 0;
    char *tok = strtok(input, " \t");
    while (tok != NULL && argc < max_tokens - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }

    /* argv 끝을 NULL로 닫아 이후 처리에서 안전하게 순회할 수 있게 한다. */
    argv[argc] = NULL;
    return argc;
}

/* 파일 크기를 천 단위 콤마가 들어간 문자열로 만든다. */
static void format_size_with_commas(off_t value, char *buf, size_t bufsize)
{
    /* 출력 버퍼가 없으면 포맷팅을 할 수 없다. */
    if (buf == NULL || bufsize == 0) return;

    /* 우선 숫자 자체를 임시 문자열로 만든다. */
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%lld", (long long)value);

    /* 부호가 있을 수 있으므로 실제 숫자 시작 위치를 따로 계산한다. */
    int len = (int)strlen(tmp);
    int neg = 0, start = 0;
    if (tmp[0] == '-') { neg = 1; start = 1; }

    /* 세 자리마다 콤마가 들어가므로 필요한 출력 길이를 미리 계산한다. */
    int digits = len - start;
    int commas = (digits - 1) / 3;
    int out_len = len + commas;
    if ((size_t)(out_len + 1) > bufsize) {
        /* 버퍼가 부족하면 콤마 없이 원래 숫자만 안전하게 출력한다. */
        snprintf(buf, bufsize, "%s", tmp);
        return;
    }

    /* 뒤에서부터 복사하면 콤마 삽입 위치를 쉽게 맞출 수 있다. */
    buf[out_len] = '\0';
    int j = out_len - 1;
    int counter = 0;
    for (int i = len - 1; i >= start; i--) {
        if (counter == 3) { buf[j--] = ','; counter = 0; }
        buf[j--] = tmp[i];
        counter++;
    }

    /* 음수 값이 들어온 경우 부호를 다시 맨 앞에 둔다. */
    if (neg) buf[0] = '-';
}

/* 권한 필터 인자를 8진수 mode 값으로 변환한다. */
static int parse_octal_mode(const char *str, mode_t *out)
{
    /* 권한 문자열과 결과 저장 위치가 모두 필요하다. */
    if (str == NULL || out == NULL) return 0;

    /* 600, 0644, 0777 같은 3~4자리 8진수 형태만 허용한다. */
    size_t len = strlen(str);
    if (len < 3 || len > 4) return 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '7') return 0;
    }

    /* 8진수로 변환한 뒤 권한 비트 범위 안인지 확인한다. */
    char *endptr = NULL;
    long v = strtol(str, &endptr, 8);
    if (*endptr != '\0' || v < 0 || v > 07777) return 0;
    *out = (mode_t)v;
    return 1;
}

/* 양의 정수 인자를 파싱한다. 세트 번호, 파일 번호, -n에 사용한다. */
static int parse_positive_int(const char *str, int *out)
{
    /* 빈 문자열과 NULL은 숫자 인자로 인정하지 않는다. */
    if (str == NULL || out == NULL || str[0] == '\0') return 0;

    /* 부호, 공백, 문자 없이 0~9만 들어 있는지 먼저 확인한다. */
    for (const char *p = str; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') return 0;
    }

    /* 실제 정수로 바꾸고 1 이상 INT_MAX 이하인지 검사한다. */
    char *endptr = NULL;
    long v = strtol(str, &endptr, 10);
    if (*endptr != '\0' || v <= 0 || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

/* 0을 포함한 정수 인자를 파싱한다. -d 0처럼 시작 디렉토리만 탐색할 때 사용한다. */
static int parse_nonnegative_int(const char *str, int *out)
{
    /* -d는 0도 허용하지만 빈 문자열은 허용하지 않는다. */
    if (str == NULL || out == NULL || str[0] == '\0') return 0;

    /* 음수 기호나 다른 문자가 있으면 깊이 값으로 쓰지 않는다. */
    for (const char *p = str; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') return 0;
    }

    /* INT 범위 안의 0 이상 값만 통과시킨다. */
    char *endptr = NULL;
    long v = strtol(str, &endptr, 10);
    if (*endptr != '\0' || v < 0 || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

/* -x COUNT는 2 이상의 정수만 허용한다. */
static int parse_min_dup_count(const char *str, int *out)
{
    /* 먼저 양의 정수인지 확인한다. */
    int v;
    if (!parse_positive_int(str, &v)) return 0;

    /* COUNT가 2보다 작으면 중복 세트 조건이 아니므로 거부한다. */
    if (v < 2) return 0;
    *out = v;
    return 1;
}

/* -r 옵션 문자열을 내부 정렬 규칙 enum으로 바꾼다. */
static int parse_sort_rule(const char *str, SortRule *out)
{
    /* 정렬 기준 문자열과 결과 저장 위치가 모두 필요하다. */
    if (str == NULL || out == NULL) return 0;

    /* 명세에서 제시한 세 가지 기준만 허용한다. */
    if (strcmp(str, "size_desc") == 0) {
        *out = SORT_SIZE_DESC;
    } else if (strcmp(str, "mtime_desc") == 0) {
        *out = SORT_MTIME_DESC;
    } else if (strcmp(str, "path_asc") == 0) {
        *out = SORT_PATH_ASC;
    } else {
        return 0;
    }
    return 1;
}

/* -u 옵션 문자열을 f/t 보존 규칙 enum으로 바꾼다. */
static int parse_preserve_rule(const char *str, PreserveRule *out)
{
    /* 보존 규칙 문자열과 결과 저장 위치가 모두 필요하다. */
    if (str == NULL || out == NULL) return 0;

    /* newest는 기존 f/t 옵션과 같은 기본 동작이다. */
    if (strcmp(str, "newest") == 0) {
        *out = PRESERVE_NEWEST;
    } else if (strcmp(str, "oldest") == 0) {
        *out = PRESERVE_OLDEST;
    } else if (strcmp(str, "pathshort") == 0) {
        *out = PRESERVE_PATHSHORT;
    } else if (strcmp(str, "pathlong") == 0) {
        *out = PRESERVE_PATHLONG;
    } else {
        return 0;
    }
    return 1;
}

/* f/t 삭제 명령 뒤에 붙는 선택적 '-u RULE'을 해석한다. */
static int parse_delete_preserve_option(char *argv[], int argc, PreserveRule *rule)
{
    /* -u가 없을 때는 기존 명세와 같은 newest를 기본값으로 둔다. */
    int idx = 2;
    int u_seen = 0;
    *rule = PRESERVE_NEWEST;

    /* f/t 뒤에 남은 토큰은 '-u RULE' 한 쌍만 허용한다. */
    while (idx < argc) {
        if (strcmp(argv[idx], "-u") != 0) return 0;
        if (u_seen) return 0;
        u_seen = 1;
        idx++;
        if (idx >= argc) return 0;
        if (!parse_preserve_rule(argv[idx], rule)) return 0;
        idx++;
    }
    return 1;
}

/* 필터 기본값은 모든 확장자, 모든 크기, 모든 시간 범위를 허용한다. */
static void filter_init(FilterContext *ctx)
{
    /* 모든 필드를 0으로 만든 뒤, 제한 없음이 기본인 필드만 별도로 켠다. */
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->ext_match_all = 1;
    ctx->min_unlimited = 1;
    ctx->max_unlimited = 1;
    ctx->sort_rule = SORT_DEFAULT;
    ctx->min_dup_count = 2;
}

/* 경로가 특정 디렉토리 자신이거나 그 하위인지 '/' 경계를 고려해 검사한다. */
static int path_under(const char *path, const char *prefix)
{
    /* prefix와 앞부분이 같고, 바로 끝나거나 '/'가 이어져야 하위 경로다. */
    size_t lp = strlen(prefix);
    if (strncmp(path, prefix, lp) != 0) return 0;
    return path[lp] == '\0' || path[lp] == '/';
}

/* 루트 탐색 금지 디렉토리와 사용자가 지정한 -e 디렉토리를 BFS에서 제외한다. */
static int filter_should_skip_dir(const FilterContext *ctx, const char *abs_dir)
{
    /* NULL 디렉토리는 열 수 없으므로 탐색에서 제외한다. */
    if (abs_dir == NULL) return 1;

    /* proc/run/sys는 가상 파일시스템이 많아 무한하거나 특수한 항목을 피하기 위해 제외한다. */
    static const char *system_dirs[] = {"/proc", "/run", "/sys", NULL};
    for (int i = 0; system_dirs[i] != NULL; i++) {
        if (path_under(abs_dir, system_dirs[i])) return 1;
    }

    /* 사용자가 -e로 지정한 디렉토리와 그 하위도 탐색 큐에서 건너뛴다. */
    if (ctx != NULL) {
        for (int i = 0; i < ctx->exclude_count; i++) {
            if (path_under(abs_dir, ctx->exclude_dirs[i])) return 1;
        }
    }
    return 0;
}

/* 파일명이 사용자가 지정한 확장자 조건을 만족하는지 확인한다. */
static int extension_match(const FilterContext *ctx, const char *abs_path)
{
    /* '*' 조건이면 파일명 확인 없이 통과시킨다. */
    if (ctx->ext_match_all) return 1;
    if (ctx->ext_pattern[0] == '\0') return 1;

    /* 절대경로에서 파일명 부분만 떼어 마지막 점 뒤를 확장자로 본다. */
    const char *name = strrchr(abs_path, '/');
    name = name ? name + 1 : abs_path;
    const char *dot = strrchr(name, '.');

    /* 점이 없거나 숨김 파일처럼 첫 글자가 점이면 확장자 없음으로 처리한다. */
    if (dot == NULL || dot == name) return 0;
    return strcmp(dot + 1, ctx->ext_pattern) == 0;
}

/* 파일 크기가 최소/최대 조건 안에 들어오는지 확인한다. */
static int size_in_range(const FilterContext *ctx, off_t size)
{
    /* -s 계열이 아니면 크기 조건을 적용하지 않는다. */
    if (!ctx->has_size_filter) return 1;

    /* 최소/최대가 '~'가 아닌 경우에만 범위 비교를 한다. */
    if (!ctx->min_unlimited && size < ctx->min_size) return 0;
    if (!ctx->max_unlimited && size > ctx->max_size) return 0;
    return 1;
}

/* atime/mtime/ctime 값이 지정된 시간 범위 안에 있는지 확인한다. */
static int time_in_range(int has, time_t v, time_t from, time_t to)
{
    /* 해당 시간 옵션이 없으면 항상 통과한다. */
    if (!has) return 1;

    /* 시간값은 from 이상, to 이하인 경우만 조건을 만족한다. */
    if (v < from) return 0;
    if (v > to)   return 0;
    return 1;
}

/* 이미 후보에 들어간 같은 inode 파일이 있는지 찾아 하드링크를 걸러낸다. */
static int has_same_inode(FileNode *existing, dev_t dev, ino_t ino)
{
    /* 이미 수집한 후보 중 같은 device/inode가 있으면 같은 하드링크 파일이다. */
    for (FileNode *p = existing; p != NULL; p = p->next) {
        if (p->device == dev && p->inode == ino) return 1;
    }
    return 0;
}

/* 정규 파일 하나가 모든 필터 조건을 만족하는지 최종 판정한다. */
static int filter_match(const FilterContext *ctx, const char *abs_path,
                        const struct stat *st, FileNode *existing)
{
    /* 필터 판단에 필요한 정보가 없으면 후보로 넣지 않는다. */
    if (ctx == NULL || abs_path == NULL || st == NULL) return 0;

    /* 명세상 중복 대상은 일반 파일만이므로 디렉토리, 링크, 장치 파일은 제외한다. */
    if (!S_ISREG(st->st_mode))                 return 0;

    /* 확장자, 크기, 시간 필터를 순서대로 적용한다. */
    if (!extension_match(ctx, abs_path))       return 0;
    if (!size_in_range(ctx, st->st_size))      return 0;
    if (!time_in_range(ctx->has_atime, st->st_atime, ctx->atime_from, ctx->atime_to)) return 0;
    if (!time_in_range(ctx->has_mtime, st->st_mtime, ctx->mtime_from, ctx->mtime_to)) return 0;
    if (!time_in_range(ctx->has_ctime, st->st_ctime, ctx->ctime_from, ctx->ctime_to)) return 0;

    /* -p가 있으면 파일 권한 비트가 정확히 같은 파일만 통과한다. */
    if (ctx->has_mode_filter) {
        if ((st->st_mode & 07777) != ctx->mode_filter) return 0;
    }

    /* -h가 있으면 같은 inode로 이미 들어온 하드링크는 제외한다. */
    if (ctx->exclude_hardlinks) {
        if (has_same_inode(existing, st->st_dev, st->st_ino)) return 0;
    }
    return 1;
}

/* BFS 큐를 빈 상태로 초기화한다. */
static void queue_init(DirQueue *q) { q->front = NULL; q->rear = NULL; }

/* BFS 큐 뒤쪽에 디렉토리 경로와 깊이를 넣는다. */
static int queue_enqueue(DirQueue *q, const char *path, int depth)
{
    /* 큐 노드를 만들고 디렉토리 경로와 현재 BFS 깊이를 저장한다. */
    DirNode *n = (DirNode *)malloc(sizeof(DirNode));
    if (n == NULL) return 0;
    snprintf(n->path, sizeof(n->path), "%s", path);
    n->depth = depth;
    n->next = NULL;

    /* 빈 큐면 front와 rear가 같은 노드를 가리키고, 아니면 rear 뒤에 붙인다. */
    if (q->rear == NULL) { q->front = n; q->rear = n; }
    else                 { q->rear->next = n; q->rear = n; }
    return 1;
}

/* BFS 큐 앞쪽에서 다음 탐색 디렉토리를 꺼낸다. */
static DirNode *queue_dequeue(DirQueue *q)
{
    /* 비어 있으면 꺼낼 디렉토리가 없다. */
    if (q->front == NULL) return NULL;

    /* front를 한 칸 앞으로 옮기고, 큐가 비면 rear도 NULL로 맞춘다. */
    DirNode *n = q->front;
    q->front = n->next;
    if (q->front == NULL) q->rear = NULL;

    /* 반환 노드는 더 이상 큐와 연결되지 않게 분리한다. */
    n->next = NULL;
    return n;
}

/* 큐가 비었는지 확인한다. */
static int queue_empty(DirQueue *q) { return q->front == NULL; }

/* 남아 있는 큐 노드를 모두 해제한다. */
static void queue_free(DirQueue *q)
{
    /* 오류 중간 반환에 대비해 큐에 남은 노드를 모두 꺼내 해제한다. */
    while (!queue_empty(q)) free(queue_dequeue(q));
}

/* 디렉토리 경로와 파일명을 안전하게 이어 붙인다. */
static int join_path(char *out, size_t out_size, const char *dir, const char *name)
{
    /* 디렉토리 끝에 '/'가 없으면 하나 추가해서 올바른 경로를 만든다. */
    size_t dlen = strlen(dir);
    int    sep = (dlen > 0 && dir[dlen - 1] != '/');

    /* snprintf 반환값으로 버퍼 잘림 여부를 확인한다. */
    int n = snprintf(out, out_size, "%s%s%s", dir, sep ? "/" : "", name);
    return (n > 0 && (size_t)n < out_size);
}

/* 시작 디렉토리부터 BFS로 내려가며 필터를 통과한 정규 파일만 후보 목록에 저장한다. */
static int bfs_collect_candidates(const char *start_dir, const FilterContext *ctx,
                                  FileNode **out_head, FileNode **out_tail)
{
    /* 탐색 시작점, 필터, 결과 목록 포인터가 모두 있어야 한다. */
    if (start_dir == NULL || ctx == NULL || out_head == NULL || out_tail == NULL) return -1;

    /* 호출자에게 빈 후보 목록부터 돌려줄 준비를 한다. */
    *out_head = NULL;
    *out_tail = NULL;

    /* BFS 시작 디렉토리를 깊이 0으로 큐에 넣는다. */
    DirQueue q;
    queue_init(&q);
    if (!queue_enqueue(&q, start_dir, 0)) return -1;

    /* 큐가 빌 때까지 디렉토리를 하나씩 꺼내 같은 깊이 순서로 탐색한다. */
    while (!queue_empty(&q)) {
        DirNode *cur = queue_dequeue(&q);

        /* 제외 디렉토리는 열어 보지 않고 즉시 버린다. */
        if (filter_should_skip_dir(ctx, cur->path)) { free(cur); continue; }

        DIR *dir = opendir(cur->path);
        if (dir == NULL) { free(cur); continue; }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            /* 현재/부모 디렉토리 항목은 순환 방지를 위해 건너뛴다. */
            if (entry->d_name[0] == '.' &&
                (entry->d_name[1] == '\0' ||
                 (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) continue;

            char full_path[SSU_PATH_MAX];
            if (!join_path(full_path, sizeof(full_path), cur->path, entry->d_name)) continue;

            struct stat st;
            /* lstat을 사용해 심볼릭 링크 자체를 판별하고 링크를 따라가지 않는다. */
            if (lstat(full_path, &st) != 0) continue;

            if (S_ISDIR(st.st_mode)) {
                /* 다음 깊이가 -d 제한을 넘으면 하위 탐색을 하지 않는다. */
                int next_depth = cur->depth + 1;
                if (ctx->opt_d_seen && next_depth > ctx->max_depth) continue;
                queue_enqueue(&q, full_path, next_depth);
            }
            else if (S_ISREG(st.st_mode)) {
                /* 일반 파일은 모든 필터를 통과할 때만 후보 링크드 리스트에 저장한다. */
                if (filter_match(ctx, full_path, &st, *out_head)) {
                    FileNode *node = filenode_create(full_path, &st);
                    if (node != NULL) {
                        filenode_list_append(out_head, out_tail, node);
                    }
                }
            }
            /* 심볼릭 링크와 기타 파일 형식은 중복 탐색 대상이 아니므로 건너뛴다. */
        }
        closedir(dir);
        free(cur);
    }
    queue_free(&q);
    return 0;
}

/* stat 시간 값을 출력 형식에 맞는 문자열로 바꾼다. */
static void format_time(time_t t, char *buf, size_t bufsize)
{
    /* localtime 실패 시에도 출력 형식이 깨지지 않도록 기본 문자열을 넣는다. */
    struct tm *tm = localtime(&t);
    if (tm == NULL) { snprintf(buf, bufsize, "0000-00-00 00:00:00"); return; }

    /* 명세 예시와 같은 YYYY-MM-DD HH:MM:SS 형식으로 출력한다. */
    strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S", tm);
}

/* 전체 중복 세트를 지정한 출력 스트림에 명세 형식으로 출력한다. */
static void print_setlist_to(FILE *out, const SetList *list)
{
    /* 출력할 중복 세트가 없으면 아무것도 출력하지 않는다. */
    if (out == NULL || list == NULL || list->head == NULL) return;

    /* 세트 번호는 현재 정렬된 링크드 리스트 순서대로 1부터 매긴다. */
    int idx = 1;
    for (FileSet *s = list->head; s != NULL; s = s->next, idx++) {
        char size_buf[64];
        format_size_with_commas(s->size, size_buf, sizeof(size_buf));

        /* 각 세트의 크기와 MD5 해시를 헤더 줄에 출력한다. */
        fprintf(out, "---- Identical files #%d (%s bytes - %s) ----\n",
                idx, size_buf, s->hash);

        /* 세트 내부 파일도 1부터 번호를 붙여 삭제 명령에서 참조할 수 있게 한다. */
        int file_idx = 1;
        for (FileNode *n = s->head; n != NULL; n = n->next, file_idx++) {
            char mtime_buf[32], atime_buf[32];
            format_time(n->mtime, mtime_buf, sizeof(mtime_buf));
            format_time(n->atime, atime_buf, sizeof(atime_buf));
            fprintf(out, "[%d] %s (mtime : %s) (atime : %s)\n",
                    file_idx, n->path, mtime_buf, atime_buf);
        }

        /* 명세 예시처럼 세트와 세트 사이에 빈 줄을 둔다. */
        fprintf(out, "\n");
    }
}

/* 기존 호출부가 표준 출력으로 결과를 찍을 때 쓰는 래퍼다. */
static void print_setlist(const SetList *list)
{
    print_setlist_to(stdout, list);
}

/* t 옵션에서 사용할 홈 디렉토리 아래 휴지통을 없으면 생성한다. */
static int ensure_trash_dir(char *out_dir, size_t out_size)
{
    /* 홈 디렉토리를 기준으로 명세의 휴지통 경로를 만든다. */
    const char *home = getenv("HOME");
    if (home == NULL) {
        /* HOME이 없으면 현재 사용자의 passwd 엔트리에서 홈 디렉토리를 찾는다. */
        struct passwd *pw = getpwuid(getuid());
        if (pw == NULL) return 0;
        home = pw->pw_dir;
    }

    /* 이미 존재하면 그대로 사용하고, 없으면 새로 생성한다. */
    snprintf(out_dir, out_size, "%s/%s", home, TRASH_DIR_NAME);
    if (mkdir(out_dir, 0755) != 0 && errno != EEXIST) return 0;
    return 1;
}

/* rename()이 EXDEV로 실패할 때를 대비해 파일 내용을 복사하고 원본 메타데이터를 보존한다. */
static int copy_file_preserve_metadata(const char *src, const char *dest)
{
    /* 원본 메타데이터를 먼저 읽어 복사 후 권한/시간 복구에 사용한다. */
    struct stat st;
    FILE *in = NULL;
    FILE *out = NULL;
    int ok = 0;

    if (stat(src, &st) != 0) return 0;

    /* 원본과 대상 파일을 바이너리 모드로 열어 내용 그대로 복사한다. */
    in = fopen(src, "rb");
    if (in == NULL) return 0;

    out = fopen(dest, "wb");
    if (out == NULL) {
        fclose(in);
        return 0;
    }

    /* 버퍼 단위로 끝까지 복사하고, 읽기/쓰기 오류가 있으면 실패로 둔다. */
    while (1) {
        unsigned char buf[HASH_READ_CHUNK];
        size_t n = fread(buf, 1, sizeof(buf), in);

        if (n > 0 && fwrite(buf, 1, n, out) != n) break;
        if (n < sizeof(buf)) {
            if (ferror(in)) break;
            ok = 1;
            break;
        }
    }

    /* fclose에서 발생한 지연 쓰기 오류도 실패로 처리한다. */
    if (fclose(out) != 0) ok = 0;
    fclose(in);

    /* 복사가 불완전하면 휴지통에 남은 부분 파일을 지운다. */
    if (!ok) {
        unlink(dest);
        return 0;
    }

    /* rename과 비슷하게 보이도록 권한과 atime/mtime을 원본 기준으로 맞춘다. */
    chmod(dest, st.st_mode & 07777);
    struct utimbuf times;
    times.actime = st.st_atime;
    times.modtime = st.st_mtime;
    utime(dest, &times);
    return 1;
}

/* 파일을 휴지통으로 이동한다. 파일시스템이 다르면 복사 후 원본 삭제로 처리한다. */
static int move_to_trash(const char *src)
{
    /* 휴지통 디렉토리를 준비하고 그 절대 경로를 받는다. */
    char trash_dir[1024];
    if (!ensure_trash_dir(trash_dir, sizeof(trash_dir))) return 0;

    /* 원래 파일명만 휴지통 안의 새 경로에 사용한다. */
    const char *raw_base = strrchr(src, '/');
    raw_base = raw_base ? raw_base + 1 : src;
    char base[SSU_NAME_MAX + 1];
    size_t bl = strnlen(raw_base, SSU_NAME_MAX);
    memcpy(base, raw_base, bl);
    base[bl] = '\0';

    char dest[SSU_PATH_MAX];
    int n = snprintf(dest, sizeof(dest), "%s/%s", trash_dir, base);
    if (n < 0 || (size_t)n >= sizeof(dest)) return 0;

    /* 같은 이름이 이미 있으면 시간 기반 접미사를 붙여 덮어쓰지 않는다. */
    struct stat st;
    if (stat(dest, &st) == 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        n = snprintf(dest, sizeof(dest), "%s/%s.%ld_%ld",
                     trash_dir, base, (long)tv.tv_sec, (long)tv.tv_usec);
        if (n < 0 || (size_t)n >= sizeof(dest)) return 0;
    }

    /* 같은 파일시스템이면 rename이 가장 안전하고 빠르다. */
    if (rename(src, dest) == 0) return 1;

    /* EXDEV가 아니면 권한/경로 등 다른 오류이므로 실패로 처리한다. */
    if (errno != EXDEV) return 0;

    /* 다른 파일시스템이면 복사 후 원본 삭제로 휴지통 이동을 흉내 낸다. */
    if (!copy_file_preserve_metadata(src, dest)) return 0;
    if (unlink(src) != 0) {
        /* 원본 삭제 실패 시 복사본을 제거해 중복 파일이 생기지 않게 한다. */
        unlink(dest);
        return 0;
    }
    return 1;
}

/* d 옵션은 선택한 세트의 LIST_IDX 파일 하나를 삭제하고 남은 세트를 다시 출력한다. */
static void handle_d(SetList *list, FileSet *target, int set_idx,
                     char *argv[], int argc, int min_dup_count)
{
    /* d 옵션은 뒤에 LIST_IDX가 반드시 필요하다. */
    if (argc < 3) { printf("Error: missing list index\n"); return; }

    /* 사용자가 입력한 파일 번호를 양의 정수로 검증한다. */
    int list_idx;
    if (!parse_positive_int(argv[2], &list_idx)) {
        printf("Error: invalid list index\n"); return;
    }

    /* 선택한 세트 안에서 해당 파일 노드를 찾는다. */
    FileNode *node = filenode_list_at(target->head, list_idx);
    if (node == NULL) { printf("Error: list index out of range\n"); return; }

    /* unlink 뒤 노드를 해제하므로 출력할 경로를 미리 복사해 둔다. */
    char path_copy[SSU_PATH_MAX];
    snprintf(path_copy, sizeof(path_copy), "%s", node->path);

    /* 실제 파일 삭제가 성공해야 목록에서도 제거한다. */
    if (unlink(path_copy) != 0) {
        printf("Error: unlink failed: %s\n", strerror(errno));
        return;
    }
    filenode_list_remove_at(&target->head, list_idx);
    target->count--;

    printf("\"%s\" has been deleted in #%d\n\n", path_copy, set_idx);

    /* 세트가 -x 기준 미만으로 줄어들면 출력 대상에서 제거한다. */
    if (target->count < min_dup_count) setlist_remove(list, target);
    print_setlist(list);
}

/* i 옵션은 세트 내부 파일마다 y/n을 물어보고 선택된 파일만 삭제한다. */
static void handle_i(SetList *list, FileSet *target, int min_dup_count)
{
    /* 세트 내부 파일을 순회하면서 각 파일 삭제 여부를 직접 입력받는다. */
    char input[INPUT_BUF_LEN];
    FileNode *prev = NULL;
    FileNode *node = target->head;
    while (node != NULL) {
        /* 현재 노드가 삭제될 수 있으므로 다음 노드를 미리 저장한다. */
        FileNode *next = node->next;

        /* y 또는 n만 유효한 답으로 처리한다. */
        printf("Delete \"%s\"? [y/n] ", node->path);
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) return;
        input[strcspn(input, "\r\n")] = '\0';

        if (strcmp(input, "y") == 0 || strcmp(input, "Y") == 0) {
            if (unlink(node->path) == 0) {
                /* 삭제 성공 시 현재 노드를 링크드 리스트에서 바로 떼어낸다. */
                if (prev == NULL) target->head = next;
                else              prev->next   = next;
                free(node);
                target->count--;
                /* 현재 노드를 제거했으므로 prev는 이전 위치를 유지한다. */
            } else {
                printf("Error: unlink failed: %s\n", strerror(errno));
                prev = node;
            }
        } else if (strcmp(input, "n") == 0 || strcmp(input, "N") == 0) {
            prev = node;
        } else {
            /* y/n 외 입력은 작업을 중단하고 서브 프롬프트로 돌아간다. */
            printf("Error: invalid answer\n");
            return;
        }
        node = next;
    }

    /* 대화형 삭제 결과와 다음 세트 출력 사이에 명세 예시처럼 빈 줄을 둔다. */
    printf("\n");

    /* 대화형 삭제 후에도 -x 기준을 만족하는 세트만 목록에 남긴다. */
    if (target->count < min_dup_count) setlist_remove(list, target);
    print_setlist(list);
}

/* f 옵션은 -u 기준에 맞는 파일 하나만 남기고 나머지를 삭제한다. */
static void handle_f(SetList *list, FileSet *target, int set_idx,
                     PreserveRule rule, int min_dup_count)
{
    /* 파일이 2개 미만이면 삭제할 중복 파일이 없다. */
    if (target->count < 2) return;

    /* 기본 newest 또는 사용자가 지정한 -u 기준으로 보존 파일을 고른다. */
    FileNode *keep = fileset_select_preserve(target, rule);
    if (keep == NULL) return;

    /* keep 노드가 나중에 남더라도 출력용 값은 미리 복사해 둔다. */
    char keep_path[SSU_PATH_MAX];
    snprintf(keep_path, sizeof(keep_path), "%s", keep->path);
    time_t keep_mtime = keep->mtime;

    /* 세트 내부를 한 번 순회하며 keep을 제외한 파일을 삭제한다. */
    FileNode *node = target->head;
    FileNode *prev = NULL;
    int all_ok = 1;
    while (node != NULL) {
        /* 현재 노드가 삭제될 수 있으므로 next를 먼저 저장한다. */
        FileNode *next = node->next;
        if (node != keep) {
            if (unlink(node->path) == 0) {
                /* 삭제 성공 파일은 리스트에서도 제거한다. */
                if (prev == NULL) target->head = next;
                else              prev->next = next;
                free(node);
                target->count--;
            } else {
                /* 삭제 실패 파일은 그대로 남기고 다음 순회를 계속한다. */
                all_ok = 0;
                prev = node;
            }
        } else {
            prev = node;
        }
        node = next;
    }

    /* 보존된 파일과 보존 기준 mtime을 사용자에게 알려 준다. */
    char tbuf[32];
    format_time(keep_mtime, tbuf, sizeof(tbuf));
    if (all_ok) printf("Left file in #%d : %s (%s)\n\n", set_idx, keep_path, tbuf);
    else        printf("Error: failed to delete some files in #%d\n\n", set_idx);

    /* keep만 남거나 -x 기준 미만이면 세트를 결과 목록에서 제거한다. */
    if (target->count < min_dup_count) setlist_remove(list, target);
    print_setlist(list);
}

/* t 옵션은 -u 기준에 맞는 파일 하나만 남기고 나머지를 휴지통으로 이동한다. */
static void handle_t(SetList *list, FileSet *target, int set_idx,
                     PreserveRule rule, int min_dup_count)
{
    /* 파일이 2개 미만이면 휴지통으로 옮길 중복 파일이 없다. */
    if (target->count < 2) return;

    /* f 옵션과 같이 기본 newest 또는 -u 기준에 맞는 파일 하나는 남긴다. */
    FileNode *keep = fileset_select_preserve(target, rule);
    if (keep == NULL) return;

    /* 출력 메시지에 쓸 보존 파일 정보를 미리 저장한다. */
    char keep_path[SSU_PATH_MAX];
    snprintf(keep_path, sizeof(keep_path), "%s", keep->path);
    time_t keep_mtime = keep->mtime;

    /* keep이 아닌 파일을 모두 휴지통으로 이동한다. */
    FileNode *node = target->head;
    FileNode *prev = NULL;
    int all_ok = 1;
    while (node != NULL) {
        /* 이동 성공 시 현재 노드를 해제할 수 있으므로 next를 먼저 잡는다. */
        FileNode *next = node->next;
        if (node != keep) {
            if (move_to_trash(node->path)) {
                /* 휴지통 이동이 끝난 파일은 현재 세트에서 제거한다. */
                if (prev == NULL) target->head = next;
                else              prev->next = next;
                free(node);
                target->count--;
            } else {
                /* 이동 실패 파일은 목록에 남겨 이후 상태를 출력하게 한다. */
                all_ok = 0;
                prev = node;
            }
        } else {
            prev = node;
        }
        node = next;
    }

    /* 휴지통 이동 결과와 남긴 파일의 수정 시간을 출력한다. */
    char tbuf[32];
    format_time(keep_mtime, tbuf, sizeof(tbuf));
    if (all_ok) {
        printf("All files in #%d have moved to Trash except \"%s\" (%s)\n\n",
               set_idx, keep_path, tbuf);
    } else {
        printf("Error: failed to move some files in #%d to Trash\n\n", set_idx);
    }

    /* 하나만 남거나 -x 기준 미만이면 더 이상 출력 대상이 아니므로 제거한다. */
    if (target->count < min_dup_count) setlist_remove(list, target);
    print_setlist(list);
}

/* 중복 탐색 결과가 있을 때 삭제 명령을 반복해서 입력받는 서브 프롬프트다. */
static void run_subprompt(SetList *list, int min_dup_count)
{
    /* 삭제 명령 한 줄, 토큰화용 복사본, argv 배열을 준비한다. */
    char  input[INPUT_BUF_LEN];
    char  copy[INPUT_BUF_LEN];
    char *argv[MAX_TOKENS];

    /* 중복 세트가 남아 있는 동안만 삭제 서브 프롬프트를 유지한다. */
    while (list != NULL && list->head != NULL) {
        printf("%s", SUB_PROMPT);
        fflush(stdout);

        /* EOF가 들어오면 더 이상 삭제 명령을 받을 수 없으므로 종료한다. */
        if (fgets(input, sizeof(input), stdin) == NULL) return;
        input[strcspn(input, "\r\n")] = '\0';
        if (input[0] == '\0') continue;

        /* strtok이 문자열을 수정하므로 원본 입력을 복사본에서 토큰화한다. */
        snprintf(copy, sizeof(copy), "%s", input);
        int argc = tokenize_line(copy, argv, MAX_TOKENS);

        /* exit는 삭제 서브 프롬프트만 빠져나가 메인 프롬프트로 돌아간다. */
        if (argc >= 1 && strcmp(argv[0], "exit") == 0) {
            printf(">> Back to Prompt\n");
            return;
        }

        /* 삭제 명령은 최소한 세트 번호와 옵션 하나가 필요하다. */
        if (argc < 2) { printf("Error: usage [SET_INDEX] [OPTION ...]\n"); continue; }

        /* 첫 토큰으로 삭제 대상 중복 세트를 찾는다. */
        int set_idx;
        if (!parse_positive_int(argv[0], &set_idx)) {
            printf("Error: invalid set index\n"); continue;
        }
        FileSet *target = setlist_at(list, set_idx);
        if (target == NULL) {
            printf("Error: set index %d out of range\n", set_idx); continue;
        }

        /* 두 번째 토큰은 d/i/f/t 중 한 글자 옵션만 허용한다. */
        const char *opt = argv[1];
        if (strlen(opt) != 1) { printf("Error: invalid option '%s'\n", opt); continue; }

        /* 실제 삭제 동작은 옵션별 핸들러에 위임한다. */
        switch (opt[0]) {
            case 'd':
                /* d는 파일 번호 하나만 추가로 받는다. */
                if (argc != 3) { printf("Error: invalid d option usage\n"); continue; }
                handle_d(list, target, set_idx, argv, argc, min_dup_count);
                break;
            case 'i':
                /* i는 추가 인자 없이 세트 전체를 대화형으로 처리한다. */
                if (argc != 2) { printf("Error: invalid i option usage\n"); continue; }
                handle_i(list, target, min_dup_count);
                break;
            case 'f': {
                /* f는 선택적으로 '-u RULE'을 받아 보존 파일 기준을 바꾼다. */
                PreserveRule rule;
                if (!parse_delete_preserve_option(argv, argc, &rule)) {
                    printf("Error: invalid -u option\n");
                    continue;
                }
                handle_f(list, target, set_idx, rule, min_dup_count);
                break;
            }
            case 't': {
                /* t도 f와 같은 -u 보존 규칙을 지원한다. */
                PreserveRule rule;
                if (!parse_delete_preserve_option(argv, argc, &rule)) {
                    printf("Error: invalid -u option\n");
                    continue;
                }
                handle_t(list, target, set_idx, rule, min_dup_count);
                break;
            }
            default:
                printf("Error: invalid option '%s'\n", opt);
                continue;
        }
    }
}

/* 첫 번째 옵션 묶음(-s, -sh, -sah 등)을 플래그 단위로 해석한다. */
static int parse_first_flag(const char *flag,
                            int *s, int *a, int *m, int *c, int *h)
{
    /* 첫 옵션은 반드시 '-'로 시작하고 뒤에 한 글자 이상 있어야 한다. */
    if (flag == NULL || flag[0] != '-' || flag[1] == '\0') return 0;

    /* 각 옵션 글자의 등장 여부를 0으로 초기화한다. */
    *s = *a = *m = *c = *h = 0;

    /* -sah처럼 붙어서 들어온 글자를 하나씩 읽어 플래그로 분해한다. */
    for (const char *p = flag + 1; *p != '\0'; p++) {
        switch (*p) {
            case 's': if (*s) return 0; *s = 1; break;
            case 'a': if (*a) return 0; *a = 1; break;
            case 'm': if (*m) return 0; *m = 1; break;
            case 'c': if (*c) return 0; *c = 1; break;
            case 'h': if (*h) return 0; *h = 1; break;
            default: return 0;
        }
    }

    /* 시간 기준은 atime/mtime/ctime 중 하나만 동시에 사용할 수 있다. */
    if ((*a + *m + *c) > 1) return 0;

    /* -h는 검색 기준 옵션과 함께 올 때만 의미가 있다. */
    if (*h && !(*s || *a || *m || *c)) return 0;

    /* 첫 옵션 묶음에는 검색 기준이 최소 하나 있어야 한다. */
    if (!(*s || *a || *m || *c)) return 0;

    return 1;
}

/* a/m/c 중 하나의 시간 필터를 컨텍스트에 저장한다. */
static int apply_time_filter(FilterContext *ctx, char which, time_t f, time_t t)
{
    /* which 값에 따라 FilterContext의 해당 시간 필드만 채운다. */
    switch (which) {
        case 'a':
            /* 같은 시간 옵션이 중복 지정되면 잘못된 입력으로 본다. */
            if (ctx->has_atime) return 0;
            ctx->has_atime = 1;
            ctx->atime_from = f; ctx->atime_to = t; break;
        case 'm':
            if (ctx->has_mtime) return 0;
            ctx->has_mtime = 1;
            ctx->mtime_from = f; ctx->mtime_to = t; break;
        case 'c':
            if (ctx->has_ctime) return 0;
            ctx->has_ctime = 1;
            ctx->ctime_from = f; ctx->ctime_to = t; break;
        default: return 0;
    }

    /* 과제 명세 기준으로 시간 조건은 한 종류만 적용되도록 제한한다. */
    if (ctx->has_atime + ctx->has_mtime + ctx->has_ctime > 1) return 0;
    return 1;
}

/* FROM과 TO 문자열을 실제 시간 범위로 바꾸고 역전된 범위를 거부한다. */
static int parse_time_range(const char *from_s, const char *to_s,
                            time_t *out_f, time_t *out_t)
{
    /* FROM은 시작 경계, TO는 종료 경계 규칙으로 각각 해석한다. */
    time_t f, t;
    ParseResult fr = parse_time_arg(from_s, &f, 1);
    ParseResult tr = parse_time_arg(to_s,   &t, 0);
    if (fr == PARSE_ERROR || tr == PARSE_ERROR) return 0;

    /* 무제한 시작은 epoch, 무제한 종료는 현재 시각으로 치환한다. */
    if (fr == PARSE_UNLIMITED) f = 0;
    if (tr == PARSE_UNLIMITED) t = time(NULL);

    /* 시작이 종료보다 늦으면 검색 범위가 모순이므로 거부한다. */
    if (f > t) return 0;

    /* 완성된 시간 범위를 호출자에게 저장한다. */
    *out_f = f; *out_t = t;
    return 1;
}

/* fmd5 전체 명령행을 파싱해 필터 컨텍스트와 시작 디렉토리를 완성한다. */
static int parse_args(int argc, char *argv[],
                      FilterContext *ctx, char *abs_dir, size_t abs_size)
{
    /* argv[0]만 있는 경우는 fmd5 옵션이 없으므로 사용법 오류다. */
    if (argc < 2) return 0;

    /* 첫 옵션 묶음에서 검색 종류와 -h 동시 지정 여부를 얻는다. */
    int has_s, has_a, has_m, has_c, has_h;
    if (!parse_first_flag(argv[1], &has_s, &has_a, &has_m, &has_c, &has_h))
        return 0;

    /* idx는 아직 해석하지 않은 다음 argv 위치를 가리킨다. */
    int idx = 2;

    if (has_s) {
        /* -s 계열은 확장자, 크기 범위, 대상 디렉토리를 먼저 읽는다. */
        if (argc - idx < 4) return 0;
        const char *ext    = argv[idx++];
        const char *minstr = argv[idx++];
        const char *maxstr = argv[idx++];
        const char *dir    = argv[idx++];

        /* 확장자 조건을 검증하고 실제 비교용 확장자 문자열을 저장한다. */
        if (!validate_extension(ext)) return 0;
        if (strcmp(ext, "*") == 0) {
            ctx->ext_match_all = 1;
            ctx->ext_pattern[0] = '\0';
        } else {
            ctx->ext_match_all = 0;
            extract_extension(ext, ctx->ext_pattern, sizeof(ctx->ext_pattern));
        }

        /* 최소 크기와 최대 크기를 각각 바이트 단위로 변환한다. */
        ParseResult mr = parse_size(minstr, &ctx->min_size);
        if (mr == PARSE_ERROR) return 0;
        ctx->min_unlimited = (mr == PARSE_UNLIMITED);

        ParseResult Mr = parse_size(maxstr, &ctx->max_size);
        if (Mr == PARSE_ERROR) return 0;
        ctx->max_unlimited = (Mr == PARSE_UNLIMITED);

        /* 둘 다 제한이 있을 때 최소값이 최대값보다 크면 잘못된 범위다. */
        if (!ctx->min_unlimited && !ctx->max_unlimited &&
            ctx->min_size > ctx->max_size) return 0;
        ctx->has_size_filter = 1;

        /* 대상 디렉토리는 '~' 확장과 realpath 검증을 마친 절대경로로 저장한다. */
        if (!resolve_target_dir(dir, abs_dir, abs_size)) return 0;

        /* 첫 옵션 묶음 안의 -h 상태를 컨텍스트에 반영한다. */
        ctx->exclude_hardlinks = has_h;

        if (has_a || has_m || has_c) {
            /* -sa/-sm/-sc처럼 붙은 시간 옵션은 대상 디렉토리 뒤의 FROM/TO를 읽는다. */
            if (argc - idx < 2) return 0;
            time_t f, t;
            if (!parse_time_range(argv[idx], argv[idx+1], &f, &t)) return 0;
            idx += 2;
            char which = has_a ? 'a' : (has_m ? 'm' : 'c');
            if (!apply_time_filter(ctx, which, f, t)) return 0;
        }
    } else {
        /* -a/-m/-c 단독 계열은 시간 범위와 대상 디렉토리만 읽는다. */
        if (argc - idx < 3) return 0;
        const char *from = argv[idx++];
        const char *to   = argv[idx++];
        const char *dir  = argv[idx++];

        /* 시간 전용 검색은 FROM/TO를 먼저 시간 범위로 만든다. */
        time_t f, t;
        if (!parse_time_range(from, to, &f, &t)) return 0;

        /* 첫 옵션이 -a/-m/-c 중 무엇이었는지에 따라 필터를 저장한다. */
        char which = has_a ? 'a' : (has_m ? 'm' : 'c');
        if (!apply_time_filter(ctx, which, f, t)) return 0;
        ctx->exclude_hardlinks = has_h;

        /* 시간 전용 검색도 마지막 인자는 대상 디렉토리다. */
        if (!resolve_target_dir(dir, abs_dir, abs_size)) return 0;
    }

    /* 남은 토큰은 시간 필터 또는 추가 기능 옵션으로 순서대로 처리한다. */
    while (idx < argc) {
        const char *opt = argv[idx++];

        if ((strcmp(opt, "-a") == 0 || strcmp(opt, "-m") == 0 ||
             strcmp(opt, "-c") == 0) && has_s) {
            /* 분리형 시간 옵션은 -s 검색 뒤에만 허용한다. */
            if (argc - idx < 2) return 0;
            time_t f, t;
            if (!parse_time_range(argv[idx], argv[idx+1], &f, &t)) return 0;
            idx += 2;
            if (!apply_time_filter(ctx, opt[1], f, t)) return 0;
        }
        else if (strcmp(opt, "-h") == 0) {
            /* -h가 이미 첫 옵션 묶음에 있었다면 중복 지정이다. */
            if (ctx->exclude_hardlinks) return 0;
            ctx->exclude_hardlinks = 1;
        }
        else if (strcmp(opt, "-d") == 0) {
            /* -d는 BFS 하위 탐색 깊이를 제한한다. */
            if (idx >= argc) return 0;
            if (ctx->opt_d_seen) return 0;
            int d;
            if (!parse_nonnegative_int(argv[idx++], &d)) return 0;
            ctx->max_depth = d;
            ctx->opt_d_seen = 1;
        }
        else if (strcmp(opt, "-n") == 0) {
            /* -n은 출력할 큰 중복 세트 개수만 남긴다. */
            if (idx >= argc) return 0;
            if (ctx->opt_n_seen) return 0;
            int n;
            if (!parse_positive_int(argv[idx++], &n)) return 0;
            ctx->max_count = n;
            ctx->opt_n_seen = 1;
        }
        else if (strcmp(opt, "-e") == 0) {
            /* -e는 한 번만 쓰고 뒤에 최대 3개 제외 디렉토리를 받는다. */
            if (idx >= argc) return 0;
            if (ctx->opt_e_seen) return 0;
            ctx->opt_e_seen = 1;

            int added = 0;
            while (idx < argc && argv[idx][0] != '-') {
                if (added >= MAX_EXCLUDE_DIRS) return 0;
                char abs_excl[SSU_PATH_MAX];
                if (!resolve_target_dir(argv[idx++], abs_excl, sizeof(abs_excl))) return 0;
                snprintf(ctx->exclude_dirs[ctx->exclude_count],
                         SSU_PATH_MAX, "%s", abs_excl);
                ctx->exclude_count++;
                added++;
            }
            if (added == 0) return 0;
        }
        else if (strcmp(opt, "-p") == 0) {
            /* -p는 파일 권한 mode와 정확히 일치하는 파일만 통과시킨다. */
            if (idx >= argc) return 0;
            if (ctx->opt_p_seen) return 0;
            if (!parse_octal_mode(argv[idx++], &ctx->mode_filter)) return 0;
            ctx->has_mode_filter = 1;
            ctx->opt_p_seen = 1;
        }
        else if (strcmp(opt, "-r") == 0) {
            /* -r은 출력할 중복 세트의 정렬 기준을 바꾼다. */
            if (idx >= argc) return 0;
            if (ctx->opt_r_seen) return 0;
            if (!parse_sort_rule(argv[idx++], &ctx->sort_rule)) return 0;
            ctx->opt_r_seen = 1;
        }
        else if (strcmp(opt, "-o") == 0) {
            /* -o는 탐색 결과를 지정한 파일에도 저장한다. */
            if (idx >= argc) return 0;
            if (ctx->opt_o_seen) return 0;
            if (!expand_output_path(argv[idx++], ctx->output_path, sizeof(ctx->output_path)))
                return 0;
            ctx->has_output_path = 1;
            ctx->opt_o_seen = 1;
        }
        else if (strcmp(opt, "-x") == 0) {
            /* -x는 파일 개수가 COUNT 이상인 세트만 결과에 남긴다. */
            if (idx >= argc) return 0;
            if (ctx->opt_x_seen) return 0;
            if (!parse_min_dup_count(argv[idx++], &ctx->min_dup_count)) return 0;
            ctx->opt_x_seen = 1;
        }
        else {
            /* 정의하지 않은 옵션은 help 출력 대상이 되도록 파싱 실패를 돌려준다. */
            return 0;
        }
    }
    return 1;
}

/* 후보 파일을 크기별로 먼저 묶고, 같은 크기 안에서 MD5 해시별 중복 세트를 만든다. */
static SetList build_setlist(FileNode **candidates_ptr, const FilterContext *ctx)
{
    /* 최종 중복 세트 목록을 빈 상태로 만든다. */
    SetList result;
    setlist_init(&result);

    /* 같은 크기 파일끼리 먼저 묶어 불필요한 해시 계산을 줄인다. */
    SetList by_size;
    setlist_init(&by_size);

    /* 후보 목록의 소유권을 이 함수로 가져와 크기 그룹에 재배치한다. */
    FileNode *cands = *candidates_ptr;
    *candidates_ptr = NULL;

    while (cands != NULL) {
        /* 현재 후보를 원래 목록에서 떼어 단독 노드로 만든다. */
        FileNode *next = cands->next;
        cands->next = NULL;

        /* 같은 크기 그룹이 이미 있는지 링크드 리스트에서 찾는다. */
        FileSet *target = NULL;
        for (FileSet *s = by_size.head; s != NULL; s = s->next) {
            if (s->size == cands->size) { target = s; break; }
        }

        /* 해당 크기 그룹이 없으면 새 임시 세트를 만든다. */
        if (target == NULL) {
            target = fileset_create(cands->size, "");
            if (target == NULL) { free(cands); cands = next; continue; }
            setlist_append(&by_size, target);
        }

        /* 후보 파일을 크기 그룹에 넣고 다음 후보로 이동한다. */
        fileset_add(target, cands);
        cands = next;
    }

    /* 크기가 같은 파일들만 MD5를 계산해 해시별 세트로 다시 묶는다. */
    while (by_size.head != NULL) {
        /* by_size의 첫 그룹을 떼어 처리한다. */
        FileSet *grp = by_size.head;
        by_size.head = grp->next;
        grp->next = NULL;
        by_size.count--;

        /* 같은 크기 파일이 1개뿐이면 중복 가능성이 없으므로 버린다. */
        if (grp->count < 2) { fileset_free(grp); continue; }

        /* 그룹 껍데기는 임시였으므로 파일 노드만 빼내고 세트 구조체는 해제한다. */
        FileNode *nodes = grp->head;
        grp->head = NULL;
        free(grp);

        /* 같은 크기 그룹 안에서 해시별 하위 그룹을 만든다. */
        SetList sub;
        setlist_init(&sub);

        while (nodes != NULL) {
            /* 현재 파일 노드를 떼어 MD5 결과에 맞는 해시 그룹으로 옮긴다. */
            FileNode *nn = nodes->next;
            nodes->next = NULL;

            char hex[MD5_HEX_LEN];
            if (!md5_compute_file(nodes->path, hex)) {
                /* 읽을 수 없는 파일은 결과에서 제외하고 노드를 해제한다. */
                free(nodes);
                nodes = nn;
                continue;
            }

            /* 같은 해시를 가진 하위 그룹이 이미 있는지 찾는다. */
            FileSet *hgrp = NULL;
            for (FileSet *s = sub.head; s != NULL; s = s->next) {
                if (strcmp(s->hash, hex) == 0) { hgrp = s; break; }
            }

            /* 처음 보는 해시면 새 세트를 만들고 해시 문자열을 저장한다. */
            if (hgrp == NULL) {
                hgrp = fileset_create(nodes->size, hex);
                if (hgrp == NULL) { free(nodes); nodes = nn; continue; }
                setlist_append(&sub, hgrp);
            }

            /* 파일 노드를 해시 그룹에 연결한다. */
            fileset_add(hgrp, nodes);
            nodes = nn;
        }

        /* 하위 해시 그룹 중 -x 기준을 만족하는 실제 중복 세트만 최종 결과로 옮긴다. */
        while (sub.head != NULL) {
            FileSet *s = sub.head;
            sub.head = s->next;
            s->next = NULL;
            if (s->count >= ctx->min_dup_count) setlist_append(&result, s);
            else                                fileset_free(s);
        }
    }

    /* 각 세트 내부와 전체 세트 목록을 명세 출력 순서로 정렬한다. */
    for (FileSet *s = result.head; s != NULL; s = s->next) {
        /* fileset_add가 앞에 붙였으므로 출력 전에 세트 내부 파일 순서를 정렬한다. */
        filenode_list_sort(&s->head);
    }

    /* -n은 먼저 큰 세트를 골라야 하므로 크기 내림차순으로 정렬한다. */
    if (ctx->max_count > 0) setlist_sort_by(&result, fileset_cmp_size_desc);
    else                    setlist_sort_with_rule(&result, ctx->sort_rule);

    /* -n 옵션이 있으면 큰 파일 세트부터 지정 개수만 남긴다. */
    if (ctx->max_count > 0 && result.count > ctx->max_count) {
        /* max_count번째 세트를 keep_tail로 잡고 그 뒤를 모두 제거한다. */
        FileSet *keep_tail = result.head;
        for (int i = 1; keep_tail != NULL && i < ctx->max_count; i++) {
            keep_tail = keep_tail->next;
        }
        if (keep_tail != NULL) {
            /* 잘라낸 나머지 세트는 메모리 누수 없이 해제한다. */
            FileSet *drop = keep_tail->next;
            keep_tail->next = NULL;
            while (drop != NULL) {
                FileSet *next = drop->next;
                fileset_free(drop);
                result.count--;
                drop = next;
            }
        }
    }

    /* -n으로 큰 세트를 고른 뒤 -r이 있으면 남은 결과를 사용자가 고른 기준으로 다시 정렬한다. */
    if (ctx->max_count > 0 && ctx->sort_rule != SORT_DEFAULT) {
        setlist_sort_with_rule(&result, ctx->sort_rule);
    }

    /* 완성된 중복 세트 목록을 값으로 반환한다. */
    return result;
}

/* fmd5 실행 진입점이다. 파싱, 탐색, 출력, 삭제 서브 프롬프트를 순서대로 수행한다. */
int main(int argc, char *argv[])
{
    /* ssu_clean과 파이프 입력을 나눠 읽기 위해 stdin 버퍼링을 끈다. */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* 명령행 옵션을 저장할 필터 구조체를 기본값으로 초기화한다. */
    FilterContext ctx;
    filter_init(&ctx);

    /* parse_args가 채워 줄 탐색 시작 디렉토리 절대경로다. */
    char abs_dir[SSU_PATH_MAX];

    if (!parse_args(argc, argv, &ctx, abs_dir, sizeof(abs_dir))) {
        /* 잘못된 인자는 help 출력으로 사용법을 다시 안내한다. */
        execl("./ssu_help", "help", (char *)NULL);
        fprintf(stderr, "fmd5: invalid arguments\n");
        return 1;
    }

    /* 탐색과 중복 세트 구성에 걸린 시간만 측정한다. */
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);

    /* BFS로 후보 파일을 모으고, 크기와 MD5를 기준으로 중복 세트를 만든다. */
    FileNode *cands = NULL, *cands_tail = NULL;
    bfs_collect_candidates(abs_dir, &ctx, &cands, &cands_tail);
    SetList sets = build_setlist(&cands, &ctx);

    /* 탐색 종료 시각을 기록하고 sec/usec 차이를 보정한다. */
    gettimeofday(&tv_end, NULL);
    long sec  = tv_end.tv_sec  - tv_start.tv_sec;
    long usec = tv_end.tv_usec - tv_start.tv_usec;
    if (usec < 0) { sec--; usec += 1000000; }

    /* -o가 있으면 탐색 결과를 저장할 파일을 새로 열거나 기존 내용을 덮어쓴다. */
    FILE *out_fp = NULL;
    if (ctx.has_output_path) {
        out_fp = fopen(ctx.output_path, "w");
        if (out_fp == NULL) {
            printf("Error: cannot open output file \"%s\": %s\n",
                   ctx.output_path, strerror(errno));
            setlist_free(&sets);
            return 1;
        }
    }

    /* 중복이 없으면 결과 없음과 검색 시간만 출력하고 끝낸다. */
    if (sets.head == NULL) {
        printf("No duplicates in %s\n", abs_dir);
        printf("Searching time: %ld:%06ld(sec:usec)\n", sec, usec);
        if (out_fp != NULL) {
            fprintf(out_fp, "No duplicates in %s\n", abs_dir);
            fprintf(out_fp, "Searching time: %ld:%06ld(sec:usec)\n", sec, usec);
            if (fclose(out_fp) != 0) {
                printf("Error: cannot write output file \"%s\": %s\n",
                       ctx.output_path, strerror(errno));
                return 1;
            }
        }
        return 0;
    }

    /* 중복 세트와 검색 시간을 먼저 출력한다. */
    print_setlist(&sets);
    printf("Searching time: %ld:%06ld(sec:usec)\n", sec, usec);

    /* -o가 있으면 표준 출력과 같은 탐색 결과를 파일에도 기록한다. */
    if (out_fp != NULL) {
        print_setlist_to(out_fp, &sets);
        fprintf(out_fp, "Searching time: %ld:%06ld(sec:usec)\n", sec, usec);
        if (fclose(out_fp) != 0) {
            printf("Error: cannot write output file \"%s\": %s\n",
                   ctx.output_path, strerror(errno));
            setlist_free(&sets);
            return 1;
        }
    }

    /* 아직 중복 세트가 남아 있으면 d/i/f/t 서브 프롬프트를 실행한다. */
    if (sets.head != NULL) {
        run_subprompt(&sets, ctx.min_dup_count);
    }

    /* 남은 동적 메모리를 모두 해제하고 fmd5 프로세스를 정상 종료한다. */
    setlist_free(&sets);
    return 0;
}
