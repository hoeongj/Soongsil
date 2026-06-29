/*
 * ssu_score.h - ssu_score 채점 시스템 메인 헤더 파일
 *
 * 역할: 채점 시스템 전체에서 사용하는 상수 정의, 구조체, 함수 프로토타입 선언.
 *       main.c, ssu_score.c 등에서 인클루드하여 공통 인터페이스를 제공한다.
 */
#ifndef MAIN_H_
#define MAIN_H_

/* ============================================================
 * 불리언 및 상수 정의
 * ============================================================ */

/* 불리언 상수: C에 내장 bool이 없으므로 매크로로 정의 */
#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif

/* 파일 디스크립터 번호 상수: dup2() 리다이렉션에 사용 */
#ifndef STDOUT
	#define STDOUT 1    /* 표준 출력 파일 디스크립터 */
#endif
#ifndef STDERR
	#define STDERR 2    /* 표준 에러 파일 디스크립터 */
#endif

/* 파일 유형 식별 상수: get_file_type() 반환값으로 사용 */
#ifndef TEXTFILE
	#define TEXTFILE 3  /* .txt 파일 (빈칸 채우기 문제) */
#endif
#ifndef CFILE
	#define CFILE 4     /* .c 파일 (프로그램 문제) */
#endif

/* 프로그램 실행 타임아웃(초): 학생 프로그램이 이 시간 초과 시 강제 종료 */
#ifndef OVER
	#define OVER 5
#endif

/* 컴파일 경고/에러 감점 상수 */
#ifndef WARNING
	#define WARNING -0.1    /* 경고 1개당 감점 점수 */
#endif
#ifndef ERROR
	#define ERROR 0         /* 에러 발생 시 반환값 (0점 처리) */
#endif

/* ============================================================
 * 버퍼/배열 크기 상수
 * ============================================================ */
#define FILELEN 64      /* 파일명 저장 버퍼 크기 */
#define BUFLEN 1024     /* 범용 문자열 버퍼 크기 */
#define SNUM 100        /* 최대 학생 수 */
#define QNUM 100        /* 최대 문제 수 */
#define ARGNUM 5        /* -t, -c 옵션 최대 인자 수 */

/* ============================================================
 * 채점표 구조체
 * ============================================================ */

/*
 * ssu_scoreTable: 문제별 배점 정보를 저장하는 구조체
 *   qname - 문제 파일명 (예: "1-1.txt", "2-1.c")
 *   score - 해당 문제의 배점
 */
struct ssu_scoreTable{
	char qname[FILELEN];
	double score;
};

/* ============================================================
 * 함수 프로토타입 선언
 * ============================================================ */

/* --- 메인 진입 및 옵션 처리 --- */
void ssu_score(int argc, char *argv[]);     /* 채점 시스템 메인 함수 */
int check_option(int argc, char *argv[]);   /* 명령줄 옵션 파싱 (-e, -t, -h, -p, -c) */
void print_usage();                          /* 사용법 출력 */

/* --- 학생 채점 --- */
void score_students();                       /* 전체 학생 채점 (병렬 파이프라인) */
double score_student(int fd, char *id);      /* 개별 학생 채점 (원본 순차 방식, 보존됨) */
void write_first_row(int fd);               /* CSV 헤더 행 작성 */

/* --- 채점 세부 함수 --- */
char *get_answer(int fd, char *result);          /* 정답 파일에서 답안 읽기 */
int score_blank(char *id, char *filename);       /* 빈칸 채우기 문제 채점 */
double score_program(char *id, char *filename);  /* 프로그램 문제 채점 (컴파일+실행+비교) */
double compile_program(char *id, char *filename);/* 프로그램 컴파일 및 에러 검사 */
int execute_program(char *id, char *filname);    /* 프로그램 실행 및 결과 비교 */
pid_t inBackground(char *name);                  /* 백그라운드 프로세스 확인 (원본 보존) */
double check_error_warning(char *filename);      /* 에러/경고 파일 분석 및 감점 계산 */
int compare_resultfile(char *file1, char *file2);/* 실행 결과 파일 비교 */

/* --- -c 옵션 관련 --- */
void do_cOption(char (*ids)[FILELEN]);           /* 특정 학생 점수 조회 */
int is_exist(char (*src)[FILELEN], char *target);/* 문자열 배열에서 대상 존재 여부 확인 */

/* --- 유틸리티 --- */
int is_thread(char *qname);                      /* -t 옵션의 스레드 문제 여부 확인 */
void redirection(char *command, int newfd, int oldfd); /* 명령 실행 + 출력 리다이렉션 */
int get_file_type(char *filename);               /* 파일 확장자로 유형 판별 (.txt/.c) */
void rmdirs(const char *path);                   /* 디렉토리 재귀 삭제 */
void to_lower_case(char *c);                     /* 문자 소문자 변환 */

/* --- 채점표/학생 테이블 관리 --- */
void set_scoreTable(char *ansDir);               /* 채점표 초기화 (파일 읽기 또는 생성) */
void read_scoreTable(char *path);                /* CSV에서 채점표 읽기 */
void make_scoreTable(char *ansDir);              /* 대화형으로 채점표 생성 */
void write_scoreTable(char *filename);           /* 채점표를 CSV로 저장 */
void set_idTable(char *stuDir);                  /* 학생 ID 테이블 초기화 */
int get_create_type();                           /* 채점표 생성 방식 선택 */

/* --- 정렬 --- */
void sort_idTable(int size);                     /* 학생 ID 테이블 버블 정렬 */
void sort_scoreTable(int size);                  /* 채점표 문제 번호순 버블 정렬 */
void get_qname_number(char *qname, int *num1, int *num2); /* 문제명에서 번호 추출 */

#endif
