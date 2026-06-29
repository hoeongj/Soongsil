/*
 * blank.c - 빈칸 채우기 문제 채점 엔진
 *
 * 역할: C 코드 문자열을 토큰화하고 구문 트리(AST)를 생성하여
 *       학생 답안과 정답의 의미적 동치성(semantic equivalence)을 판별한다.
 *       교환 법칙, 관계 연산자 반전, 포인터/형변환 등 C 문법 특수 케이스를 처리한다.
 *
 * [개선 요약]
 *   - is_commutative_op(): 교환 법칙 연산자 판별 헬퍼 추가
 *   - compare_tree(): 관계연산자/==/!= 판별 최적화
 *   - create_node(): malloc 1회 통합 (노드+이름 메모리)
 *   - get_root(): 재귀 -> 반복 변환
 *   - clear_tokens(): 개별 memset -> 단일 memset
 *   - rtrim(): 댕글링 포인터 버그 수정
 *   - make_tokens(): TOKEN_CNT 경계 검사 추가
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"

/*
 * datatype 배열: C 언어에서 인식하는 데이터 타입 키워드 목록
 *
 * make_tokens()에서 타입 선언 문장을 식별하고,
 * 포인터('*')가 타입 뒤에 올 때 타입에 붙여서 처리하는 데 사용한다.
 * 총 DATATYPE_SIZE(35)개의 타입을 지원한다.
 */
char datatype[DATATYPE_SIZE][MINLEN] = {"int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct"};


/*
 * operators 배열: 연산자 우선순위 테이블
 *
 * 각 연산자의 precedence 값은 낮을수록 높은 우선순위를 의미한다.
 * 괄호('(', ')')는 0으로 특수 처리된다.
 * 구문 트리 생성 시 연산자 삽입 위치를 결정하는 데 사용한다.
 *
 * 우선순위 그룹:
 *   0: 괄호
 *   1: 멤버 접근 (->)
 *   2-4: 산술 (%, /, *)
 *   5-6: 산술 (+, -)
 *   7: 관계 (<, <=, >, >=)
 *   8: 동등 (==, !=)
 *   9-11: 비트 (&, ^, |)
 *   12-13: 논리 (&&, ||)
 *   14: 대입 (=, +=, -=, &=, |=)
 */
operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}
	,{"->", 1}
	,{"*", 4}	,{"/", 3}	,{"%", 2}
	,{"+", 6}	,{"-", 5}
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}
	,{"==", 8}	,{"!=", 8}
	,{"&", 9}
	,{"^", 10}
	,{"|", 11}
	,{"&&", 12}
	,{"||", 13}
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}
};

/*
 * [개선 추가] is_commutative_op() - 교환 법칙이 성립하는 연산자인지 판별
 *
 * 교환 법칙(commutative property)이 성립하는 연산자: +, *, |, &, ||, &&
 * 이들은 피연산자 순서를 바꿔도 결과가 동일하므로,
 * 학생 답안과 정답의 피연산자 순서가 달라도 정답으로 인정해야 한다.
 *
 * 최적화: 첫 글자(name[0])로 빠르게 분기하여 O(1) 시간에 판별한다.
 *         원본은 compare_tree() 내에서 매번 strlen+strcmp 조합으로 확인했으나,
 *         이 헬퍼를 사용하여 코드 중복을 제거하고 가독성을 높였다.
 *
 * 단일 문자 연산자: +, *, |, & (name[1] == '\0' 확인)
 * 이중 문자 연산자: ||, && (name[0..1] 확인 + name[2] == '\0')
 *
 * 반환: 교환 가능하면 1(true), 아니면 0(false)
 */
static int is_commutative_op(const char *name)
{
	char c = name[0];
	/* 단일 문자 교환 연산자: +, *, |, & */
	if(name[1] == '\0')
		return (c == '+' || c == '*' || c == '|' || c == '&');
	/* 이중 문자 교환 연산자: ||, && */
	return (name[0] == '|' && name[1] == '|' && name[2] == '\0')
		|| (name[0] == '&' && name[1] == '&' && name[2] == '\0');
}

/*
 * [개선] compare_tree() - 두 구문 트리의 의미적 동치성 비교
 *
 * 개선 내용:
 *   1) 관계연산자 판별 최적화: '<', '>' 문자의 첫 글자(name[0])를 먼저 확인하여
 *      불필요한 strncmp 호출을 줄였다. 원본은 매번 strncmp를 4회 호출했다.
 *   2) ==/!= 판별 최적화: name[0]이 '=' 또는 '!'이고 name[1]이 '='인지
 *      직접 확인하여 strcmp 호출을 제거했다.
 *   3) is_commutative_op() 사용: 교환 법칙 연산자를 판별하는 코드를
 *      헬퍼 함수로 추출하여 가독성을 높이고 코드 중복을 제거했다.
 *
 * 비교 전략:
 *   - 관계연산자(<, >, <=, >=): 반대 방향으로 변환 후 피연산자 교환하여 비교
 *   - 동등 연산자(==, !=): 피연산자 순서를 바꿔서 재비교
 *   - 교환 법칙 연산자(+, *, |, &, ||, &&): 모든 피연산자 조합으로 비교
 *   - 기타 연산자: 순서 그대로 비교
 *
 * 매개변수:
 *   root1  - 학생 답안의 구문 트리 루트
 *   root2  - 정답의 구문 트리 루트
 *   result - 비교 결과 (true/false) 저장 포인터
 */
void compare_tree(node *root1,  node *root2, int *result)
{
	node *tmp;
	int cnt1, cnt2;
	char c;

	/* 어느 한쪽이 NULL이면 불일치 */
	if(root1 == NULL || root2 == NULL){
		*result = false;
		return;
	}

	/*
	 * [개선] 관계연산자 처리 최적화
	 * 첫 글자가 '<' 또는 '>'이고 유효한 관계연산자인 경우,
	 * 학생 답안의 연산자와 정답의 연산자가 다르면 반대 방향으로 변환하고
	 * 피연산자를 교환하여 비교한다.
	 * 예: a < b == b > a
	 */
	c = root1->name[0];
	if((c == '<' || c == '>') && (root1->name[1] == '\0' || root1->name[1] == '=')){
		if(strcmp(root1->name, root2->name) != 0){
			/* 정답의 관계연산자를 반대 방향으로 변환 */
			if(!strncmp(root2->name, "<", 1))
				strncpy(root2->name, ">", 1);

			else if(!strncmp(root2->name, ">", 1))
				strncpy(root2->name, "<", 1);

			else if(!strncmp(root2->name, "<=", 2))
				strncpy(root2->name, ">=", 2);

			else if(!strncmp(root2->name, ">=", 2))
				strncpy(root2->name, "<=", 2);

			/* 피연산자(자식) 순서 교환 */
			root2 = change_sibling(root2);
		}
	}

	/* 노드 이름이 다르면 불일치 */
	if(strcmp(root1->name, root2->name) != 0){
		*result = false;
		return;
	}

	/* 자식 유무가 다르면 불일치 (한쪽만 자식이 있는 경우) */
	if((root1->child_head != NULL && root2->child_head == NULL)
		|| (root1->child_head == NULL && root2->child_head != NULL)){
		*result = false;
		return;
	}

	/* 양쪽 모두 자식이 있으면 자식 트리 비교 */
	else if(root1->child_head != NULL){
		/* 자식 수가 다르면 불일치 */
		cnt1 = get_sibling_cnt(root1->child_head);
		cnt2 = get_sibling_cnt(root2->child_head);
		if(cnt1 != cnt2){
			*result = false;
			return;
		}

		/*
		 * [개선] ==, != 연산자 판별 최적화
		 * 첫 글자가 '=' 또는 '!'이고 두 번째 글자가 '='이면
		 * 동등 비교 연산자로 판별한다. strcmp 호출 없이 직접 문자 비교.
		 * 동등 연산자는 피연산자 순서를 바꿔서 재비교 가능하다.
		 */
		c = root1->name[0];
		if((c == '=' && root1->name[1] == '=') || (c == '!' && root1->name[1] == '='))
		{
			/* 먼저 순서 그대로 비교 */
			compare_tree(root1->child_head, root2->child_head, result);

			if(*result == false)
			{
				/* 실패 시 피연산자 순서를 바꿔서 재비교 */
				*result = true;
				root2 = change_sibling(root2);
				compare_tree(root1->child_head, root2->child_head, result);
			}
		}
		/* [개선] is_commutative_op() 사용하여 교환 법칙 연산자 판별 */
		else if(is_commutative_op(root1->name))
		{
			/* 교환 법칙: 정답의 모든 피연산자 순서 조합을 시도 */
			tmp = root2->child_head;

			/* 첫 번째 형제까지 이동 */
			while(tmp->prev != NULL)
				tmp = tmp->prev;

			/* 각 형제와 비교 시도 */
			while(tmp != NULL)
			{
				compare_tree(root1->child_head, tmp, result);

				if(*result == true)
					break;  /* 일치하는 조합 발견 */
				else{
					if(tmp->next != NULL)
						*result = true;  /* 다음 시도를 위해 result 복원 */
					tmp = tmp->next;
				}
			}
		}
		else{
			/* 비교환 연산자: 순서 그대로 비교 */
			compare_tree(root1->child_head, root2->child_head, result);
		}
	}


	/* 형제(next) 노드가 있으면 형제 트리도 비교 */
	if(root1->next != NULL){

		/* 형제 수가 다르면 불일치 */
		if(get_sibling_cnt(root1) != get_sibling_cnt(root2)){
			*result = false;
			return;
		}

		if(*result == true)
		{
			/* 부모 연산자가 교환 법칙을 만족하면 모든 순서 조합 시도 */
			tmp = get_operator(root1);

			if(is_commutative_op(tmp->name))
			{
				tmp = root2;

				while(tmp->prev != NULL)
					tmp = tmp->prev;

				while(tmp != NULL)
				{
					compare_tree(root1->next, tmp, result);

					if(*result == true)
						break;
					else{
						if(tmp->next != NULL)
							*result = true;
						tmp = tmp->next;
					}
				}
			}

			else
				/* 비교환 연산자: 순서 그대로 비교 */
				compare_tree(root1->next, root2->next, result);
		}
	}
}

/*
 * [개선] make_tokens() - C 코드 문자열을 토큰 배열로 분리
 *
 * 개선 내용:
 *   1) row >= TOKEN_CNT-1 경계 검사를 2곳에 추가하여 토큰 배열 오버플로를 방지한다.
 *      원본은 경계 검사 없이 row를 증가시켜 배열 범위를 벗어날 수 있었다.
 *      - while 루프 마지막: row++하기 전에 TOKEN_CNT-1 이상이면 false 반환
 *      - strlen(start) 루프 내: 공백으로 토큰이 분리될 때 경계 검사
 *
 * 토큰화 규칙:
 *   - 연산자(op 문자열에 정의): 별도 토큰으로 분리
 *   - ++/--: 전위/후위에 따라 피연산자에 붙여서 처리
 *   - ==, !=, <=, >= 등 2글자 연산자: 하나의 토큰으로 처리
 *   - ->: 구조체 멤버 접근, 좌측 토큰에 붙여서 처리
 *   - &: 주소 연산(&a) vs 비트 AND(a&b) 구분
 *   - *: 포인터(*a) vs 곱셈(a*b) 구분
 *   - (): 함수 호출 vs 형변환 vs 우선순위 그룹 구분
 *   - "": 문자열 리터럴은 하나의 토큰
 *
 * 매개변수:
 *   str    - 토큰화할 C 코드 문자열
 *   tokens - 결과 토큰을 저장할 2D 배열 [TOKEN_CNT][MINLEN]
 * 반환: 성공 시 true, 실패(잘못된 구문) 시 false
 */
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN])
{
	char *start, *end;
	char tmp[BUFLEN];
	char str2[BUFLEN];
	char *op = "(),;><=!|&^/+-*\"";  /* 연산자 및 구분자 문자 집합 */
	int row = 0;
	int i;
 	int isPointer;
	int lcount, rcount;
	int p_str;

	clear_tokens(tokens);  /* 토큰 배열 초기화 */

	start = str;

	/* 타입 선언문인지 확인 (0이면 파싱 불가) */
	if(is_typeStatement(str) == 0)
		return false;

	/* 연산자/구분자를 찾아가며 토큰 분리 */
	while(1)
	{
		/* 다음 연산자/구분자 위치 탐색 */
		if((end = strpbrk(start, op)) == NULL)
			break;

		if(start == end){
			/* 현재 위치가 바로 연산자인 경우 */

			/* ++, -- 전위/후위 증감 연산자 처리 */
			if(!strncmp(start, "--", 2) || !strncmp(start, "++", 2)){
				/* ++++, ---- 같은 잘못된 구문 거부 */
				if(!strncmp(start, "++++", 4)||!strncmp(start,"----",4))
					return false;

				// ex) ++a (전위 증감)
				if(is_character(*ltrim(start + 2))){
					if(row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]))
						return false; //ex) ++a++ (잘못된 구문)

					end = strpbrk(start + 2, op);
					if(end == NULL)
						end = &str[strlen(str)];
					while(start < end) {
						if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
							return false;
						else if(*start != ' ')
							strncat(tokens[row], start, 1);
						start++;
					}
				}
				// ex) a++ (후위 증감)
				else if(row>0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){
					/* 이미 ++/--가 붙어있으면 거부 (a++++ 같은 경우) */
					if(strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL)
						return false;

					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row - 1], tmp);  /* 이전 토큰에 ++/-- 붙이기 */
					start += 2;
					row--;
				}
				else{
					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row], tmp);
					start += 2;
				}
			}

			/* 2글자 연산자 처리: ==, !=, <=, >=, ||, &&, +=, -=, 등 */
			else if(!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2)
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2)
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2)	|| !strncmp(start, "-=", 2)
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)){

				strncpy(tokens[row], start, 2);
				start += 2;
			}
			/* 구조체 멤버 접근 연산자 (->) 처리: 좌측 토큰에 붙여서 저장 */
			else if(!strncmp(start, "->", 2))
			{
				end = strpbrk(start + 2, op);

				if(end == NULL)
					end = &str[strlen(str)];

				/* -> 이후의 멤버명을 이전 토큰에 붙이기 */
				while(start < end){
					if(*start != ' ')
						strncat(tokens[row - 1], start, 1);
					start++;
				}
				row--;
			}
			/* & 연산자: 주소 연산(&a) vs 비트 AND(a&b) 구분 */
			else if(*end == '&')
			{
				// ex) &a (주소 연산: 첫 토큰이거나 앞이 연산자일 때)
				if(row == 0 || (strpbrk(tokens[row - 1], op) != NULL)){
					end = strpbrk(start + 1, op);
					if(end == NULL)
						end = &str[strlen(str)];

					strncat(tokens[row], start, 1);  /* '&' 추가 */
					start++;

					/* 뒤따르는 변수명을 같은 토큰에 붙이기 */
					while(start < end){
						if(*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&')
							return false;
						else if(*start != ' ')
							strncat(tokens[row], start, 1);
						start++;
					}
				}
				// ex) a & b (비트 AND: 앞이 피연산자일 때)
				else{
					strncpy(tokens[row], start, 1);
					start += 1;
				}

			}
			/* * 연산자: 포인터(*a) vs 곱셈(a*b) 구분 */
		  	else if(*end == '*')
			{
				isPointer=0;

				if(row > 0)
				{
					/* 데이터 타입 뒤의 *는 포인터 (예: char**, int*) */
					//ex) char** (pointer)
					for(i = 0; i < DATATYPE_SIZE; i++) {
						if(strstr(tokens[row - 1], datatype[i]) != NULL){
							strcat(tokens[row - 1], "*");  /* 타입 토큰에 * 붙이기 */
							start += 1;
							isPointer = 1;
							break;
						}
					}
					if(isPointer == 1)
						continue;
					if(*(start+1) !=0)
						end = start + 1;

					// ex) a * **b (곱셈 뒤 포인터)
					if(row>1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)){
						strncat(tokens[row - 1], start, end - start);
						row--;
					}

					// ex) a*b (곱셈: 앞이 영숫자)
					else if(is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1){
						strncat(tokens[row], start, end - start);
					}

					// ex) ,*b (포인터: 앞이 연산자)
					else if(strpbrk(tokens[row - 1], op) != NULL){
						strncat(tokens[row] , start, end - start);

					}
					else
						strncat(tokens[row], start, end - start);

					start += (end - start);
				}

			 	else if(row == 0)
				{
					/* 첫 토큰이 *인 경우: 포인터 역참조 */
					if((end = strpbrk(start + 1, op)) == NULL){
						strncat(tokens[row], start, 1);
						start += 1;
					}
					else{
						while(start < end){
							if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
								return false;
							else if(*start != ' ')
								strncat(tokens[row], start, 1);
							start++;
						}
						if(all_star(tokens[row]))
							row--;

					}
				}
			}
			/* '(' 처리: 함수 호출, 형변환, 우선순위 그룹 */
			else if(*end == '(')
			{
				lcount = 0;
				rcount = 0;
				/* &(...) 또는 *(...) 패턴 (함수 포인터 등) */
				if(row>0 && (strcmp(tokens[row - 1],"&") == 0 || strcmp(tokens[row - 1], "*") == 0)){
					/* 중첩 괄호 카운트 */
					while(*(end + lcount + 1) == '(')
						lcount++;
					start += lcount;

					end = strpbrk(start + 1, ")");

					if(end == NULL)
						return false;
					else{
						while(*(end + rcount +1) == ')')
							rcount++;
						end += rcount;

						if(lcount != rcount)
							return false;  /* 괄호 짝 불일치 */

						if( (row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1){
							/* 역참조/주소 연산자에 괄호 내용 붙이기 */
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1);
							row--;
							start = end + 1;
						}
						else{
							strncat(tokens[row], start, 1);
							start += 1;
						}
					}

				}
				else{
					/* 일반 괄호: 별도 토큰으로 저장 */
					strncat(tokens[row], start, 1);
					start += 1;
				}

			}
			/* 문자열 리터럴 ("...") 처리: 하나의 토큰으로 저장 */
			else if(*end == '\"')
			{
				end = strpbrk(start + 1, "\"");

				if(end == NULL)
					return false;  /* 닫는 따옴표 없음 */

				else{
					strncat(tokens[row], start, end - start + 1);
					start = end + 1;
				}

			}

			else{
				// ex) a++ ++ +b (잘못된 구문)
				if(row > 0 && !strcmp(tokens[row - 1], "++"))
					return false;

				// ex) a-- -- -b (잘못된 구문)
				if(row > 0 && !strcmp(tokens[row - 1], "--"))
					return false;

				strncat(tokens[row], start, 1);
				start += 1;

				// 단항 연산자 판별: -a, +b, --a, ++b 등
				if(!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")){


					// ex) -a 또는 -a+b (단항: 첫 토큰)
					if(row == 0)
						row--;

					// ex) a+b = -c (단항: 앞이 피연산자가 아닐 때)
					else if(!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){

						if(strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL)
							row--;
					}
				}
			}
		}
		else{
			/* 연산자 사이의 피연산자(변수명, 숫자 등) 처리 */

			/* 포인터 '*'를 다음 피연산자에 붙이기 위한 row 조정 */
			if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))
				row--;

			if(all_star(tokens[row - 1]) && row == 1)
				row--;

			/* 피연산자 문자를 현재 토큰에 추가 (공백이면 토큰 분리) */
			for(i = 0; i < end - start; i++){
				if(i > 0 && *(start + i) == '.'){
					/* 구조체 멤버 접근(.) 처리 */
					strncat(tokens[row], start + i, 1);

					while( *(start + i +1) == ' ' && i< end - start )
						i++;
				}
				else if(start[i] == ' '){
					while(start[i] == ' ')
						i++;
					break;  /* 공백으로 토큰 분리 */
				}
				else
					strncat(tokens[row], start + i, 1);
			}

			if(start[0] == ' '){
				start += i;
				continue;
			}
			start += i;
		}

		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		/* 연속된 피연산자/타입 처리 (예: int a, unsigned int) */
		 if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])
				&& (is_typeStatement(tokens[row - 1]) == 2
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.' ) ){

			if(row > 1 && strcmp(tokens[row - 2],"(") == 0)
			{
				if(strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1],"unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}

		}

		/* gcc 명령어는 토큰화하지 않고 통째로 저장 */
		if((row == 0 && !strcmp(tokens[row], "gcc")) ){
			clear_tokens(tokens);
			strcpy(tokens[0], str);
			return 1;
		}

		/*
		 * [개선] TOKEN_CNT-1 경계 검사 (1번째 위치)
		 * 토큰 수가 최대 용량에 도달하면 false 반환하여
		 * 배열 범위 밖 접근(out-of-bounds)을 방지한다.
		 */
		if(row >= TOKEN_CNT - 1)
			return false;
		row++;
	}

	/* 마지막 포인터 '*' 병합 처리 */
	if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))
		row--;
	if(all_star(tokens[row - 1]) && row == 1)
		row--;

	/* 남은 문자열(연산자 없는 부분)을 토큰으로 처리 */
	for(i = 0; i < strlen(start); i++)
	{
		if(start[i] == ' ')
		{
			while(start[i] == ' ')
				i++;
			if(start[0]==' ') {
				start += i;
				i = 0;
			}
			else{
				/*
				 * [개선] TOKEN_CNT-1 경계 검사 (2번째 위치)
				 * 공백으로 토큰이 분리될 때에도 경계를 검사한다.
				 */
				if(row >= TOKEN_CNT - 1)
					return false;
				row++;
			}

			i--;
		}
		else
		{
			strncat(tokens[row], start + i, 1);
			if( start[i] == '.' && i<strlen(start)){
				while(start[i + 1] == ' ' && i < strlen(start))
					i++;

			}
		}
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		/* -lpthread 특수 처리: "-" + "lpthread" -> "-lpthread" 병합 */
		if(!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")){
			strcat(tokens[row - 1], tokens[row]);
			memset(tokens[row], 0, sizeof(tokens[row]));
			row--;
		}
		/* 연속된 피연산자/타입 처리 (위와 동일한 로직) */
	 	else if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])
				&& (is_typeStatement(tokens[row - 1]) == 2
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.') ){

			if(row > 1 && strcmp(tokens[row-2],"(") == 0)
			{
				if(strcmp(tokens[row-1], "struct") != 0 && strcmp(tokens[row-1], "unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		}
	}


	if(row > 0)
	{

		// #include, struct 같은 특수 토큰은 전체를 하나의 토큰으로 재결합
		if(strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0){
			clear_tokens(tokens);
			strcpy(tokens[0], remove_extraspace(str));
		}
	}

	/* 타입 선언 또는 extern 키워드 시 모든 토큰을 하나로 결합 */
	if(is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL){
		for(i = 1; i < TOKEN_CNT; i++){
			if(strcmp(tokens[i],"") == 0)
				break;

			if(i != TOKEN_CNT -1 )
				strcat(tokens[0], " ");
			strcat(tokens[0], tokens[i]);
			memset(tokens[i], 0, sizeof(tokens[i]));
		}
	}

	/* 형변환 패턴 정규화: (char)a, (int*)b 등을 단일 토큰으로 변환 */
	//change ( ' char ' )' a  ->  (char)a
	while((p_str = find_typeSpecifier(tokens)) != -1){
		if(!reset_tokens(p_str, tokens))
			return false;
	}

	/* sizeof 구조체 패턴 정규화: sizeof(record) 등 */
	//change sizeof ' ( ' record ' ) '-> sizeof(record)
	while((p_str = find_typeSpecifier2(tokens)) != -1){
		if(!reset_tokens(p_str, tokens))
			return false;
	}

	return true;
}

/*
 * make_tree() - 토큰 배열로부터 구문 트리(AST)를 재귀적으로 생성
 *
 * 연산자 우선순위에 따라 트리를 구성한다:
 *   - 연산자: 부모 노드
 *   - 피연산자: 자식 노드
 *   - 괄호: 재귀 호출로 하위 트리 생성
 *
 * 교환 법칙 연산자(+, *, |, &, ||, &&)의 경우,
 * 같은 연산자가 연속되면 하나의 부모 아래 여러 자식을 배치한다.
 *
 * 매개변수:
 *   root        - 현재 트리 루트 (초기 NULL)
 *   tokens      - 토큰 배열
 *   idx         - 현재 토큰 인덱스 (포인터로 공유)
 *   parentheses - 현재 괄호 깊이
 * 반환: 생성된 (서브)트리의 루트 노드
 */
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses)
{
	node *cur = root;
	node *new;
	node *saved_operator;
	node *operator;
	int fstart;
	int i;

	while(1)
	{
		/* 빈 토큰이면 끝 */
		if(strcmp(tokens[*idx], "") == 0)
			break;

		/* ')' 만나면 현재 괄호 그룹 종료, 루트 반환 */
		if(!strcmp(tokens[*idx], ")"))
			return get_root(cur);

		/* ',' 만나면 함수 인자 구분, 현재 서브트리 반환 */
		else if(!strcmp(tokens[*idx], ","))
			return get_root(cur);

		/* '(' 처리: 함수 호출 또는 우선순위 그룹 */
		else if(!strcmp(tokens[*idx], "("))
		{
			// 함수 호출: 앞에 피연산자(함수명)가 있는 경우
			if(*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0){
				fstart = true;

				/* 함수 인자들을 순회하며 자식 트리 생성 */
				while(1)
				{
					*idx += 1;

					if(!strcmp(tokens[*idx], ")"))
						break;

					/* 각 인자를 재귀적으로 서브트리 생성 */
					new = make_tree(NULL, tokens, idx, parentheses + 1);

					if(new != NULL){
						if(fstart == true){
							/* 첫 번째 인자: 함수의 자식으로 연결 */
							cur->child_head = new;
							new->parent = cur;

							fstart = false;
						}
						else{
							/* 이후 인자: 형제로 연결 */
							cur->next = new;
							new->prev = cur;
						}

						cur = new;
					}

					if(!strcmp(tokens[*idx], ")"))
						break;
				}
			}
			else{
				/* 우선순위 그룹: 재귀적으로 괄호 내부 서브트리 생성 */
				*idx += 1;

				new = make_tree(NULL, tokens, idx, parentheses + 1);

				if(cur == NULL)
					cur = new;

				/* 동일 연산자 병합: 같은 교환 법칙 연산자면 자식 합류 */
				else if(!strcmp(new->name, cur->name)){
					if(!strcmp(new->name, "|") || !strcmp(new->name, "||")
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))
					{
						cur = get_last_child(cur);

						if(new->child_head != NULL){
							new = new->child_head;

							new->parent->child_head = NULL;
							new->parent = NULL;
							new->prev = cur;
							cur->next = new;
						}
					}
					else if(!strcmp(new->name, "+") || !strcmp(new->name, "*"))
					{
						i = 0;

						/* 다음 연산자를 미리 확인하여 병합 여부 결정 */
						while(1)
						{
							if(!strcmp(tokens[*idx + i], ""))
								break;

							if(is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)
								break;

							i++;
						}

						if(get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{
							cur = get_last_child(cur);
							cur->next = new;
							new->prev = cur;
							cur = new;
						}
						else
						{
							cur = get_last_child(cur);

							if(new->child_head != NULL){
								new = new->child_head;

								new->parent->child_head = NULL;
								new->parent = NULL;
								new->prev = cur;
								cur->next = new;
							}
						}
					}
					else{
						cur = get_last_child(cur);
						cur->next = new;
						new->prev = cur;
						cur = new;
					}
				}

				else
				{
					cur = get_last_child(cur);

					cur->next = new;
					new->prev = cur;

					cur = new;
				}
			}
		}
		/* 연산자 토큰 처리 */
		else if(is_operator(tokens[*idx]))
		{
			/* 교환 법칙 연산자: 동일 연산자 병합 처리 */
			if(!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
					|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&")
					|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{
				if(is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;

				else
				{
					new = create_node(tokens[*idx], parentheses);
					operator = get_most_high_precedence_node(cur, new);

					if(operator->parent == NULL && operator->prev == NULL){

						if(get_precedence(operator->name) < get_precedence(new->name)){
							cur = insert_node(operator, new);
						}

						else if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
						else
						{
							operator = cur;

							while(1)
							{
								if(is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))
									break;

								if(operator->prev != NULL)
									operator = operator->prev;
								else
									break;
							}

							if(strcmp(operator->name, tokens[*idx]) != 0)
								operator = operator->parent;

							if(operator != NULL){
								if(!strcmp(operator->name, tokens[*idx]))
									cur = operator;
							}
						}
					}

					else
						cur = insert_node(operator, new);
				}

			}
			/* 비교환 연산자: 우선순위에 따라 트리 삽입 */
			else
			{
				new = create_node(tokens[*idx], parentheses);

				if(cur == NULL)
					cur = new;

				else
				{
					operator = get_most_high_precedence_node(cur, new);

					if(operator->parentheses > new->parentheses)
						cur = insert_node(operator, new);

					else if(operator->parent == NULL && operator->prev ==  NULL){

						if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){

								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}

						else
							cur = insert_node(operator, new);
					}

					else
						cur = insert_node(operator, new);
				}
			}
		}
		/* 피연산자(변수, 상수 등) 처리 */
		else
		{
			new = create_node(tokens[*idx], parentheses);

			if(cur == NULL)
				cur = new;  /* 첫 번째 노드 */

			else if(cur->child_head == NULL){
				/* 현재 노드(연산자)의 첫 번째 자식으로 연결 */
				cur->child_head = new;
				new->parent = cur;

				cur = new;
			}
			else{
				/* 마지막 자식의 형제로 연결 */

				cur = get_last_child(cur);

				cur->next = new;
				new->prev = cur;

				cur = new;
			}
		}

		*idx += 1;
	}

	return get_root(cur);
}

/*
 * change_sibling() - 부모 노드의 두 자식 순서를 교환
 *
 * 이진 연산자의 좌/우 피연산자를 교환한다.
 * compare_tree()에서 교환 법칙 및 관계연산자 반전 비교 시 사용.
 *
 * 교환 전: parent -> [A] <-> [B]
 * 교환 후: parent -> [B] <-> [A]
 */
node *change_sibling(node *parent)
{
	node *tmp;

	tmp = parent->child_head;

	/* 두 번째 자식을 첫 번째로 승격 */
	parent->child_head = parent->child_head->next;
	parent->child_head->parent = parent;
	parent->child_head->prev = NULL;

	/* 원래 첫 번째를 두 번째로 이동 */
	parent->child_head->next = tmp;
	parent->child_head->next->prev = parent->child_head;
	parent->child_head->next->next = NULL;
	parent->child_head->next->parent = NULL;

	return parent;
}

/*
 * [개선] create_node() - 새 구문 트리 노드 생성
 *
 * 개선 내용: malloc 1회 통합 할당
 *   원본: malloc으로 node를 할당하고, 별도로 strdup(또는 malloc)으로 name을 할당
 *         -> free 시 name을 별도로 해제해야 하여 메모리 누수 위험
 *   개선: sizeof(node) + strlen(name) + 1 크기로 한 번에 할당
 *         node 구조체 바로 뒤에 name 문자열을 저장 (new + 1 위치)
 *         -> free(new) 한 번으로 노드와 이름 모두 해제, 메모리 누수 해결
 *         -> 캐시 지역성(cache locality)도 향상
 *
 * 매개변수:
 *   name        - 노드가 나타내는 토큰 문자열
 *   parentheses - 이 노드의 괄호 깊이
 * 반환: 새로 생성된 노드 포인터
 */
node *create_node(char *name, int parentheses)
{
	node *new;
	int len = strlen(name);

	/* 노드 구조체와 문자열을 하나의 메모리 블록으로 통합 할당 */
	new = (node *)malloc(sizeof(node) + len + 1);
	new->name = (char *)(new + 1);       /* name은 node 바로 뒤 공간에 위치 */
	memcpy(new->name, name, len + 1);    /* 널 종료 포함 복사 */

	/* 모든 포인터 초기화 */
	new->parentheses = parentheses;
	new->parent = NULL;
	new->child_head = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new;
}

/*
 * get_precedence() - 연산자의 우선순위 값 반환
 *
 * operators 배열에서 해당 연산자를 찾아 precedence 값을 반환한다.
 * 인덱스 2부터 검색 (0,1은 괄호).
 * 못 찾으면 false(0) 반환.
 */
int get_precedence(char *op)
{
	int i;

	for(i = 2; i < OPERATOR_CNT; i++){
		if(!strcmp(operators[i].operator, op))
			return operators[i].precedence;
	}
	return false;
}

/*
 * is_operator() - 주어진 문자열이 연산자인지 확인
 *
 * operators 배열 전체를 순회하여 일치하는 항목이 있으면 true 반환.
 */
int is_operator(char *op)
{
	int i;

	for(i = 0; i < OPERATOR_CNT; i++)
	{
		if(operators[i].operator == NULL)
			break;
		if(!strcmp(operators[i].operator, op)){
			return true;
		}
	}

	return false;
}

/*
 * print() - 구문 트리를 디버깅용으로 출력
 *
 * 재귀적으로 자식과 형제를 순회하며 노드 이름을 출력한다.
 * 개발/디버깅 시 트리 구조를 확인하는 데 사용.
 */
void print(node *cur)
{
	if(cur->child_head != NULL){
		print(cur->child_head);
		printf("\n");
	}

	if(cur->next != NULL){
		print(cur->next);
		printf("\t");
	}
	printf("%s", cur->name);
}

/*
 * get_operator() - 현재 노드의 부모(연산자) 노드 반환
 *
 * 현재 노드에서 prev 체인을 따라 첫 형제까지 이동한 뒤
 * parent를 반환한다. compare_tree()에서 교환 법칙 판별 시 사용.
 */
node *get_operator(node *cur)
{
	if(cur == NULL)
		return cur;

	/* prev 체인을 따라 첫 번째 형제로 이동 */
	if(cur->prev != NULL)
		while(cur->prev != NULL)
			cur = cur->prev;

	return cur->parent;  /* 부모 노드(연산자) 반환 */
}

/*
 * [개선] get_root() - 현재 노드에서 루트까지 올라가기
 *
 * 개선 내용: 재귀 -> 반복 변환
 *   원본: 재귀 호출로 prev/parent를 따라 올라감
 *         -> 깊은 트리에서 스택 오버플로 위험, 함수 호출 오버헤드
 *   개선: while 루프로 prev/parent를 따라 반복적으로 올라감
 *         -> 스택 사용 없음, 임의 깊이 트리에서도 안전
 *
 * 반환: 트리의 루트 노드
 */
node *get_root(node *cur)
{
	if(cur == NULL)
		return cur;

	/* 반복문으로 루트까지 올라가기 */
	while(1){
		/* 먼저 prev 체인을 따라 첫 형제로 이동 */
		while(cur->prev != NULL)
			cur = cur->prev;
		/* parent가 없으면 루트 */
		if(cur->parent == NULL)
			break;
		/* parent로 한 단계 올라가기 */
		cur = cur->parent;
	}

	return cur;
}

/*
 * get_high_precedence_node() - 우선순위가 높은 노드 탐색
 *
 * cur에서 시작하여 prev/parent 방향으로 올라가며
 * new보다 우선순위가 높은(precedence 값이 작은) 연산자 노드를 찾는다.
 * make_tree()에서 새 연산자의 삽입 위치를 결정할 때 사용.
 */
node *get_high_precedence_node(node *cur, node *new)
{
	if(is_operator(cur->name))
		if(get_precedence(cur->name) < get_precedence(new->name))
			return cur;

	if(cur->prev != NULL){
		while(cur->prev != NULL){
			cur = cur->prev;

			return get_high_precedence_node(cur, new);
		}


		if(cur->parent != NULL)
			return get_high_precedence_node(cur->parent, new);
	}

	if(cur->parent == NULL)
		return cur;
}

/*
 * get_most_high_precedence_node() - 트리 전체에서 가장 높은 우선순위 노드 탐색
 *
 * get_high_precedence_node()를 반복 호출하여 루트까지 올라가며
 * 가장 높은 우선순위를 가진 연산자 노드를 찾는다.
 */
node *get_most_high_precedence_node(node *cur, node *new)
{
	node *operator = get_high_precedence_node(cur, new);
	node *saved_operator = operator;

	while(1)
	{
		if(saved_operator->parent == NULL)
			break;

		if(saved_operator->prev != NULL)
			operator = get_high_precedence_node(saved_operator->prev, new);

		else if(saved_operator->parent != NULL)
			operator = get_high_precedence_node(saved_operator->parent, new);

		saved_operator = operator;
	}

	return saved_operator;
}

/*
 * insert_node() - 기존 노드 위치에 새 연산자 노드를 삽입
 *
 * old 노드를 new 노드의 자식으로 만들고,
 * old의 이전 형제가 있으면 new의 이전 형제로 연결한다.
 * 우선순위에 따른 트리 재구성에 사용.
 */
node *insert_node(node *old, node *new)
{
	/* old의 이전 형제를 new의 이전 형제로 연결 */
	if(old->prev != NULL){
		new->prev = old->prev;
		old->prev->next = new;
		old->prev = NULL;
	}

	/* old를 new의 자식으로 설정 */
	new->child_head = old;
	old->parent = new;

	return new;
}

/*
 * get_last_child() - 현재 노드의 마지막 자식(형제 리스트 끝) 반환
 *
 * child_head에서 시작하여 next 체인을 따라 마지막까지 이동한다.
 */
node *get_last_child(node *cur)
{
	if(cur->child_head != NULL)
		cur = cur->child_head;

	while(cur->next != NULL)
		cur = cur->next;

	return cur;
}

/*
 * get_sibling_cnt() - 형제 노드 수 반환
 *
 * 현재 노드에서 첫 형제까지 이동한 뒤 next를 따라가며 카운트.
 * 현재 노드는 포함하지 않는다.
 * compare_tree()에서 자식 수 일치 여부 확인에 사용.
 */
int get_sibling_cnt(node *cur)
{
	int i = 0;

	while(cur->prev != NULL)
		cur = cur->prev;

	while(cur->next != NULL){
		cur = cur->next;
		i++;
	}

	return i;
}

/*
 * free_node() - 노드와 하위 트리를 재귀적으로 메모리 해제
 *
 * 자식(child_head)과 형제(next)를 재귀적으로 방문하여 해제한다.
 * create_node()에서 통합 할당했으므로 free(cur) 한 번으로
 * 노드와 이름 문자열이 모두 해제된다.
 */
void free_node(node *cur)
{
	if(cur->child_head != NULL)
		free_node(cur->child_head);

	if(cur->next != NULL)
		free_node(cur->next);

	if(cur != NULL){
		/* 포인터 초기화 후 해제 (안전을 위한 방어적 코딩) */
		cur->prev = NULL;
		cur->next = NULL;
		cur->parent = NULL;
		cur->child_head = NULL;
		free(cur);  /* 노드 + name 한 번에 해제 (통합 할당이므로) */
	}
}


/*
 * is_character() - 문자가 영숫자(알파벳 또는 숫자)인지 확인
 *
 * 토큰화 시 피연산자(변수명, 숫자)를 식별하는 데 사용.
 * 반환: 영숫자이면 1(true), 아니면 0(false)
 */
int is_character(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/*
 * is_typeStatement() - 문자열이 C 데이터 타입 선언인지 확인
 *
 * 공백을 제거한 문자열과 datatype 배열을 비교한다.
 *
 * 반환값:
 *   0 - 타입 선언이 아님 (또는 위치 불일치)
 *   1 - 타입 선언 아닌 일반 문장
 *   2 - 타입 키워드로 시작 (int, char, gcc 등)
 */
int is_typeStatement(char *str)
{
	char *start;
	char str2[BUFLEN] = {0};
	char tmp[BUFLEN] = {0};
	char tmp2[BUFLEN] = {0};
	int i;

	start = str;
	strncpy(str2,str,strlen(str));
	remove_space(str2);  /* 공백 제거된 사본 생성 */

	/* 선행 공백 건너뛰기 */
	while(start[0] == ' ')
		start += 1;

	/* gcc 명령어 확인 */
	if(strstr(str2, "gcc") != NULL)
	{
		strncpy(tmp2, start, strlen("gcc"));
		if(strcmp(tmp2,"gcc") != 0)
			return 0;  /* gcc가 문자열 시작이 아님 */
		else
			return 2;  /* gcc로 시작 */
	}

	/* 각 데이터 타입과 비교 */
	for(i = 0; i < DATATYPE_SIZE; i++)
	{
		if(strstr(str2,datatype[i]) != NULL)
		{
			strncpy(tmp, str2, strlen(datatype[i]));
			strncpy(tmp2, start, strlen(datatype[i]));

			if(strcmp(tmp, datatype[i]) == 0)
				if(strcmp(tmp, tmp2) != 0)
					return 0;  /* 타입이 있지만 위치 불일치 */
				else
					return 2;  /* 타입 키워드로 시작 */
		}

	}
	return 1;  /* 타입 선언 아님, 일반 문장 */

}

/*
 * find_typeSpecifier() - 형변환(type cast) 패턴 탐색
 *
 * (type)expr 패턴을 찾아 해당 인덱스를 반환한다.
 * 예: tokens = ["(", "int", ")", "a"] -> 1 반환 (int의 인덱스)
 * 못 찾으면 -1 반환.
 */
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN])
{
	int i, j;

	for(i = 0; i < TOKEN_CNT; i++)
	{
		for(j = 0; j < DATATYPE_SIZE; j++)
		{
			if(strstr(tokens[i], datatype[j]) != NULL && i > 0)
			{
				/* (타입) 뒤에 피연산자가 오는 패턴 확인 */
				if(!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")")
						&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*'
							|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '('
							|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+'
							|| is_character(tokens[i + 2][0])))
					return i;
			}
		}
	}
	return -1;
}

/*
 * find_typeSpecifier2() - struct 형변환 패턴 탐색
 *
 * struct 키워드 뒤에 식별자가 오는 패턴을 찾아 인덱스 반환.
 * sizeof(struct xxx) 같은 패턴 처리에 사용.
 */
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN])
{
    int i, j;


    for(i = 0; i < TOKEN_CNT; i++)
    {
        for(j = 0; j < DATATYPE_SIZE; j++)
        {
            if(!strcmp(tokens[i], "struct") && (i+1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))
                    return i;
        }
    }
    return -1;
}

/*
 * all_star() - 문자열이 모두 '*'로만 구성되어 있는지 확인
 *
 * 포인터 연산자('*', '**' 등)를 식별하는 데 사용.
 * 빈 문자열은 0 반환.
 */
int all_star(char *str)
{
	int i;
	int length= strlen(str);

 	if(length == 0)
		return 0;

	for(i = 0; i < length; i++)
		if(str[i] != '*')
			return 0;
	return 1;

}

/*
 * all_character() - 문자열에 영숫자가 하나라도 포함되어 있는지 확인
 *
 * 반환: 영숫자 포함 시 1, 아니면 0
 */
int all_character(char *str)
{
	int i;

	for(i = 0; i < strlen(str); i++)
		if(is_character(str[i]))
			return 1;
	return 0;

}

/*
 * reset_tokens() - 형변환 패턴 발견 시 토큰 재배치 (정규화)
 *
 * find_typeSpecifier/find_typeSpecifier2에서 찾은 패턴을
 * 단일 토큰으로 병합한다.
 * 예: "(", "int", ")", "a" -> "(int)a"
 *
 * struct와 unsigned 키워드도 특별히 처리한다.
 * sizeof 패턴은 변환하지 않고 건너뛴다.
 */
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN])
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;
	int sub_lcount = 0, sub_rcount = 0;

	if(start > -1){
		/* struct 키워드: 다음 토큰(구조체명)과 병합 */
		if(!strcmp(tokens[start], "struct")) {
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start+1]);

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

		/* unsigned 키워드: 다음 두 토큰과 병합 */
		else if(!strcmp(tokens[start], "unsigned") && strcmp(tokens[start+1], ")") != 0) {
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start + 1]);
			strcat(tokens[start], tokens[start + 2]);

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

		/* 오른쪽 닫는 괄호 수 카운트 */
     		j = start + 1;
        	while(!strcmp(tokens[j], ")")){
                	rcount ++;
                	if(j==TOKEN_CNT)
                        	break;
                	j++;
        	}

		/* 왼쪽 여는 괄호 수 카운트 */
		j = start - 1;
		while(!strcmp(tokens[j], "(")){
        	        lcount ++;
                	if(j == 0)
                        	break;
               		j--;
		}
		if( (j!=0 && is_character(tokens[j][strlen(tokens[j])-1]) ) || j==0)
			lcount = rcount;

		/* 괄호 짝이 안 맞으면 실패 */
		if(lcount != rcount )
			return false;

		/* sizeof 패턴은 변환하지 않음 */
		if( (start - lcount) >0 && !strcmp(tokens[start - lcount - 1], "sizeof")){
			return true;
		}

		/* unsigned/struct + 형변환 토큰 병합 */
		else if((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start+1], ")")) {
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);

			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]);
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));
			}


		}
 		else{
			/* 중첩 괄호 처리 */
			if(tokens[start + 2][0] == '('){
				j = start + 2;
				while(!strcmp(tokens[j], "(")){
					sub_lcount++;
					j++;
				}
				if(!strcmp(tokens[j + 1],")")){
					j = j + 1;
					while(!strcmp(tokens[j], ")")){
						sub_rcount++;
						j++;
					}
				}
				else
					return false;

				if(sub_lcount != sub_rcount)
					return false;

				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);
				for(int i = start + 3; i<TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0]));

			}
			/* 일반 형변환 토큰 병합: (type)expr -> 단일 토큰 */
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);

			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount -1; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount +1]);
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));

			}
		}
	}
	return true;
}

/*
 * [개선] clear_tokens() - 토큰 배열 전체 초기화
 *
 * 개선 내용: 단일 memset으로 전체 배열을 한 번에 0으로 초기화.
 *   원본: 각 행마다 개별 memset을 호출하여 TOKEN_CNT번 반복
 *   개선: memset(tokens, 0, TOKEN_CNT * MINLEN) 한 번으로 처리
 *         -> 함수 호출 오버헤드 제거, 메모리 초기화 최적화
 */
void clear_tokens(char tokens[TOKEN_CNT][MINLEN])
{
	memset(tokens, 0, TOKEN_CNT * MINLEN);
}

/*
 * [개선] rtrim() - 문자열 오른쪽 공백 제거 (in-place 수정)
 *
 * 개선 내용: 댕글링 포인터 버그 수정
 *   원본: 새 문자열을 동적 할당(strdup)하여 반환 -> 호출자가 free하지 않으면
 *         메모리 누수, 반환된 포인터를 다른 곳에서 사용하면 댕글링 포인터 위험
 *   개선: 입력 문자열을 in-place로 수정하여 반환
 *         -> 메모리 할당 없음, 댕글링 포인터 불가, 메모리 누수 불가
 *
 * 동작: 문자열 끝에서부터 공백(isspace)이 아닌 문자를 찾아
 *       그 다음 위치에 '\0'을 기록한다.
 */
char *rtrim(char *_str)
{
	char *end;

	if(strlen(_str) == 0)
		return _str;

	/* 끝에서부터 공백이 아닌 문자 탐색 */
	end = _str + strlen(_str) - 1;
	while(end >= _str && isspace(*end))
		--end;

	*(end + 1) = '\0';  /* 공백 영역을 잘라내기 (in-place) */
	return _str;
}

/*
 * ltrim() - 문자열 왼쪽 공백 제거
 *
 * 문자열의 시작 부분에서 공백을 건너뛰고 첫 비공백 문자의 포인터를 반환한다.
 * 원본 문자열은 수정하지 않는다 (포인터만 이동).
 */
char *ltrim(char *_str)
{
	char *start = _str;

	while(*start != '\0' && isspace(*start))
		++start;
	_str = start;
	return _str;
}

/*
 * remove_extraspace() - 연속 공백을 단일 공백으로 축소
 *
 * #include <xxx> 같은 문장에서 불필요한 공백을 정리한다.
 * "include<" 패턴이 있으면 공백을 삽입하여 정규화한다.
 *
 * 주의: malloc으로 새 문자열을 할당하여 반환한다 (해제 필요).
 */
char* remove_extraspace(char *str)
{
	int i;
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN);
	char *start, *end;
	char temp[BUFLEN] = "";
	int position;

	/* "include<" -> "include <" 정규화 */
	if(strstr(str,"include<")!=NULL){
		start = str;
		end = strpbrk(str, "<");
		position = end - start;

		strncat(temp, str, position);
		strncat(temp, " ", 1);
		strncat(temp, str + position, strlen(str) - position + 1);

		str = temp;
	}

	/* 연속 공백을 단일 공백으로 축소 */
	for(i = 0; i < strlen(str); i++)
	{
		if(str[i] ==' ')
		{
			if(i == 0 && str[0] ==' ')
				while(str[i + 1] == ' ')
					i++;
			else{
				if(i > 0 && str[i - 1] != ' ')
					str2[strlen(str2)] = str[i];
				while(str[i + 1] == ' ')
					i++;
			}
		}
		else
			str2[strlen(str2)] = str[i];
	}

	return str2;
}



/*
 * remove_space() - 문자열에서 모든 공백 제거 (in-place)
 *
 * 두 포인터(i=쓰기, j=읽기)를 사용하여 공백을 건너뛰며 복사한다.
 * is_typeStatement()에서 타입 비교 전에 공백을 제거하는 데 사용.
 */
void remove_space(char *str)
{
	char* i = str;
	char* j = str;

	while(*j != 0)
	{
		*i = *j++;
		if(*i != ' ')
			i++;
	}
	*i = 0;
}

/*
 * check_brackets() - 괄호 짝이 맞는지 확인
 *
 * 문자열에서 '('와 ')'의 개수를 세어 같으면 1(유효), 다르면 0(무효) 반환.
 * score_blank()에서 학생 답안의 기본 문법 검증에 사용.
 */
int check_brackets(char *str)
{
	char *start = str;
	int lcount = 0, rcount = 0;

	while(1){
		if((start = strpbrk(start, "()")) != NULL){
			if(*(start) == '(')
				lcount++;
			else
				rcount++;

			start += 1;
		}
		else
			break;
	}

	if(lcount != rcount)
		return 0;  /* 괄호 짝 불일치 */
	else
		return 1;  /* 괄호 짝 일치 */
}

/*
 * get_token_cnt() - 토큰 배열에서 비어있지 않은 토큰 수 반환
 *
 * 디버깅 및 토큰 처리 시 활성 토큰 수를 확인하는 데 사용.
 */
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	for(i = 0; i < TOKEN_CNT; i++)
		if(!strcmp(tokens[i], ""))
			break;

	return i;
}
