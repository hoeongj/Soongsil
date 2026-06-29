/*
 * blank.h - 빈칸 채우기 문제 채점 엔진 헤더 파일
 *
 * 역할: C 코드 토큰화, 구문 트리(AST) 생성, 정답 비교를 위한
 *       상수, 구조체, 함수 프로토타입을 정의한다.
 *       학생 답안과 정답의 의미적 동치성(semantic equivalence)을 판별한다.
 */
#ifndef BLANK_H_
#define BLANK_H_

/* ============================================================
 * 불리언 및 공용 상수 정의
 * ============================================================ */

/* 불리언 상수 */
#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif

/* 범용 버퍼 크기 (ssu_score.h와 공유) */
#ifndef BUFLEN
	#define BUFLEN 1024
#endif

/* ============================================================
 * blank 모듈 전용 상수
 * ============================================================ */

#define OPERATOR_CNT 24     /* 지원하는 연산자의 총 개수 (operators 배열 크기) */
#define DATATYPE_SIZE 35    /* 인식하는 C 데이터 타입의 총 개수 (datatype 배열 크기) */

/*
 * [개선] MINLEN 64 -> 128로 변경
 * 원래 64바이트였으나, 긴 토큰(예: 복잡한 포인터 선언, 긴 변수명)에서
 * 버퍼 오버플로가 발생할 수 있어 128로 확대하여 안전성을 보장한다.
 */
#define MINLEN 128

#define TOKEN_CNT 50        /* 하나의 표현식에서 생성 가능한 최대 토큰 수 */

/* ============================================================
 * 구문 트리 노드 구조체
 * ============================================================ */

/*
 * node: 구문 트리(AST)의 각 노드를 나타내는 구조체
 *
 *   parentheses - 이 노드가 속한 괄호 깊이 (중첩 수준)
 *   name        - 노드가 나타내는 토큰 문자열 (연산자, 피연산자 등)
 *   parent      - 부모 노드 포인터 (연산자 노드)
 *   child_head  - 첫 번째 자식 노드 포인터
 *   prev        - 같은 부모의 이전 형제 노드 (이중 연결 리스트)
 *   next        - 같은 부모의 다음 형제 노드 (이중 연결 리스트)
 *
 * 트리 구조: 연산자가 부모, 피연산자가 자식으로 배치된다.
 * 형제 노드는 이중 연결 리스트로 연결되어 순서 교환이 용이하다.
 */
typedef struct node{
	int parentheses;
	char *name;
	struct node *parent;
	struct node *child_head;
	struct node *prev;
	struct node *next;
}node;

/*
 * operator_precedence: 연산자와 그 우선순위를 저장하는 구조체
 *
 *   operator   - 연산자 문자열 (예: "+", "==", "&&")
 *   precedence - 우선순위 값 (낮을수록 높은 우선순위, 0은 괄호)
 */
typedef struct operator_precedence{
	char *operator;
	int precedence;
}operator_precedence;

/* ============================================================
 * 트리 비교 및 조작 함수
 * ============================================================ */

/* 두 구문 트리를 비교하여 의미적 동치성 판별. 교환 법칙 고려 */
void compare_tree(node *root1,  node *root2, int *result);

/* 토큰 배열로부터 구문 트리를 재귀적으로 구성 */
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses);

/* 부모 노드의 두 자식 순서를 교환 (교환 법칙 비교용) */
node *change_sibling(node *parent);

/* 새 노드를 생성하여 반환. malloc 1회로 노드+이름 통합 할당 */
node *create_node(char *name, int parentheses);

/* 연산자 문자열의 우선순위 값 반환 */
int get_precedence(char *op);

/* 주어진 문자열이 연산자인지 확인 */
int is_operator(char *op);

/* 트리 내용을 디버깅용으로 출력 */
void print(node *cur);

/* 현재 노드의 부모(연산자) 노드를 반환 */
node *get_operator(node *cur);

/* 현재 노드로부터 루트 노드까지 올라가서 반환 */
node *get_root(node *cur);

/* 우선순위가 높은 노드를 찾아 반환 */
node *get_high_precedence_node(node *cur, node *new);

/* 트리 전체에서 가장 높은 우선순위 노드를 찾아 반환 */
node *get_most_high_precedence_node(node *cur, node *new);

/* 기존 노드 위치에 새 연산자 노드를 삽입 */
node *insert_node(node *old, node *new);

/* 현재 노드의 마지막 자식(형제 리스트 끝) 반환 */
node *get_last_child(node *cur);

/* 노드와 하위 트리를 재귀적으로 메모리 해제 */
void free_node(node *cur);

/* 형제 노드 수 반환 (현재 노드 제외) */
int get_sibling_cnt(node *cur);

/* ============================================================
 * 토큰화 및 문자열 처리 함수
 * ============================================================ */

/* C 코드 문자열을 토큰 배열로 분리. 연산자/피연산자/포인터 등 구분 */
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN]);

/* 문자열이 데이터 타입 선언인지 확인 (0: 아님, 1: 일반, 2: 타입 키워드) */
int is_typeStatement(char *str);

/* 토큰에서 형변환(type cast) 패턴을 찾아 인덱스 반환 */
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]);

/* 토큰에서 struct 형변환 패턴을 찾아 인덱스 반환 */
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]);

/* 문자가 영숫자(알파벳/숫자)인지 확인 */
int is_character(char c);

/* 문자열이 모두 '*'로 구성되어 있는지 확인 (포인터 판별) */
int all_star(char *str);

/* 문자열에 영숫자가 하나라도 포함되어 있는지 확인 */
int all_character(char *str);

/* 형변환 패턴 발견 시 토큰을 재배치하여 정규화 */
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]);

/* 토큰 배열 전체를 0으로 초기화 */
void clear_tokens(char tokens[TOKEN_CNT][MINLEN]);

/* 토큰 배열에서 비어있지 않은 토큰 수 반환 */
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]);

/* 문자열 오른쪽 공백 제거 (in-place) */
char *rtrim(char *_str);

/* 문자열 왼쪽 공백 제거 (시작 위치 포인터 반환) */
char *ltrim(char *_str);

/* 문자열에서 모든 공백 제거 (in-place) */
void remove_space(char *str);

/* 괄호 짝이 맞는지 확인 (열림/닫힘 개수 비교) */
int check_brackets(char *str);

/* 연속 공백을 단일 공백으로 축소하여 반환 */
char* remove_extraspace(char *str);

#endif
