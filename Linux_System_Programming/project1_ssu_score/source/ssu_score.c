/* 채점 시스템 핵심 모듈
 *
 * 역할: 학생 답안을 정답과 비교하여 채점하고, 결과를 score.csv로 출력한다.
 *       빈칸 채우기(.txt) 문제와 프로그램(.c) 문제를 모두 처리한다.
 *
 * [개선 요약]
 *   - 순차 채점 -> 병렬 채점 (pthread 기반)
 *   - 사전 컴파일/실행 파이프라인 (fork/execvp)
 *   - 정답 캐싱으로 반복 I/O 제거
 *   - system() -> fork/execvp 전환 (스레드 안전성)
 *   - 블록 단위 파일 읽기 최적화
 *   - 각종 버그 수정 (fclose 누락, 타임아웃 처리 등)
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ssu_score.h"
#include "blank.h"

/*
 * [개선 추가] <sys/wait.h>
 * waitpid(), WNOHANG, WIFEXITED, WEXITSTATUS, WIFSIGNALED 매크로를 사용하기 위해 필요.
 * 원본은 암묵적 선언에 의존했으나, 개선본에서 fork/exec 기반 프로세스 관리를 명시적으로
 * 수행하므로 반드시 포함해야 한다.
 */
#include <sys/wait.h>

/*
 * [개선 추가] <pthread.h>
 * POSIX 스레드(pthread_create, pthread_join)를 사용한 병렬 채점을 위해 필요.
 * 원본은 순차적으로 학생을 한 명씩 채점했으나, 개선본에서는 각 학생을
 * 별도 스레드에서 동시에 채점하여 멀티코어 활용도를 높인다.
 */
#include <pthread.h>

/* 전역 변수 외부 선언 및 정의 */
extern struct ssu_scoreTable score_table[QNUM];  /* 문제별 배점 테이블 */
extern char id_table[SNUM][10];                  /* 학생 ID 테이블 */

struct ssu_scoreTable score_table[QNUM];  /* 문제별 배점 정보 (qname, score) */
char id_table[SNUM][10];                  /* 학생 ID 목록 (정렬된 상태) */

char stuDir[BUFLEN];                      /* 학생 답안 디렉토리 경로 */
char ansDir[BUFLEN];                      /* 정답 디렉토리 경로 */
char errorDir[BUFLEN];                    /* -e 옵션: 에러 파일 저장 디렉토리 */
char threadFiles[ARGNUM][FILELEN];        /* -t 옵션: pthread 컴파일 대상 문제명 */
char cIDs[ARGNUM][FILELEN];               /* -c 옵션: 점수 조회 대상 학생 ID */

/* 옵션 플래그 */
int eOption = false;   /* -e: 에러 파일을 별도 디렉토리에 저장 */
int tOption = false;   /* -t: 특정 문제를 -lpthread로 컴파일 */
int pOption = false;   /* -p: 학생별 점수 및 평균 출력 */
int cOption = false;   /* -c: 특정 학생 점수 조회 */

/*
 * ssu_score() - 채점 시스템의 메인 엔트리 함수
 *
 * 동작 흐름:
 *   1) -h 옵션 확인 -> 사용법 출력 후 종료
 *   2) 학생/정답 디렉토리 경로를 argv에서 추출
 *   3) 옵션 파싱 (check_option)
 *   4) 디렉토리 유효성 검증 (chdir로 절대 경로 획득)
 *   5) 채점표/학생 테이블 초기화
 *   6) score_students()로 전체 채점 수행
 *   7) -c 옵션 시 특정 학생 점수 출력
 */
void ssu_score(int argc, char *argv[])
{
	char saved_path[BUFLEN];
	int i;

	/* -h 옵션이 있으면 사용법 출력 후 즉시 반환 */
	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-h")){
			print_usage();
			return;
		}
	}

	memset(saved_path, 0, BUFLEN);
	/* argv[1]=학생 디렉토리, argv[2]=정답 디렉토리 (단, -c 단독 사용 시 제외) */
	if(argc >= 3 && strcmp(argv[1], "-c") != 0){
		strcpy(stuDir, argv[1]);
		strcpy(ansDir, argv[2]);
	}

	/* 명령줄 옵션 파싱. 실패 시 종료 */
	if(!check_option(argc, argv))
		exit(1);

	/* -c 옵션만 있을 경우: 기존 score.csv에서 점수 조회 후 반환 */
	if(!eOption && !tOption && !pOption && cOption){
		do_cOption(cIDs);
		return;
	}

	/* 현재 작업 디렉토리 저장 */
	getcwd(saved_path, BUFLEN);

	/* 학생 디렉토리 유효성 검증 및 절대 경로 획득 */
	if(chdir(stuDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", stuDir);
		return;
	}
	getcwd(stuDir, BUFLEN);  /* 상대 경로를 절대 경로로 변환 */

	/* 정답 디렉토리 유효성 검증 및 절대 경로 획득 */
	chdir(saved_path);
	if(chdir(ansDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", ansDir);
		return;
	}
	getcwd(ansDir, BUFLEN);  /* 상대 경로를 절대 경로로 변환 */

	chdir(saved_path);  /* 원래 디렉토리로 복귀 */

	set_scoreTable(ansDir);  /* 채점표 초기화 (파일 읽기 또는 대화형 생성) */
	set_idTable(stuDir);     /* 학생 ID 테이블 초기화 (디렉토리 탐색) */

	printf("grading student's test papers..\n");
	score_students();  /* 전체 채점 실행 (개선: 병렬 파이프라인) */

	/* -c 옵션이 함께 있으면 특정 학생 점수도 조회 */
	if(cOption)
		do_cOption(cIDs);

	return;
}

/*
 * check_option() - 명령줄 옵션 파싱
 *
 * getopt()를 사용하여 -e, -t, -h, -p, -c 옵션을 처리한다.
 * -e: 에러 디렉토리 생성 (기존 있으면 재생성)
 * -t: -lpthread로 컴파일할 문제 목록 수집
 * -p: 점수 출력 모드 활성화
 * -c: 점수 조회 대상 학생 ID 수집
 *
 * 반환: 성공 시 true, 미지원 옵션 시 false
 */
int check_option(int argc, char *argv[])
{
	int i, j;
	int c;

	while((c = getopt(argc, argv, "e:thpc")) != -1)
	{
		switch(c){
			case 'e':
				eOption = true;
				strcpy(errorDir, optarg);

				/* 에러 디렉토리가 이미 있으면 삭제 후 재생성 */
				if(access(errorDir, F_OK) < 0)
					mkdir(errorDir, 0755);
				else{
					rmdirs(errorDir);
					mkdir(errorDir, 0755);
				}
				break;
			case 't':
				tOption = true;
				i = optind;
				j = 0;

				/* -t 뒤의 문제명들을 threadFiles 배열에 저장 */
				while(i < argc && argv[i][0] != '-'){

					if(j >= ARGNUM)
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else
						strcpy(threadFiles[j], argv[i]);
					i++;
					j++;
				}
				break;
			case 'p':
				pOption = true;
				break;
			case 'c':
				cOption = true;
				i = optind;
				j = 0;

				/* -c 뒤의 학생 ID들을 cIDs 배열에 저장 */
				while(i < argc && argv[i][0] != '-'){

					if(j >= ARGNUM)
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else
						strcpy(cIDs[j], argv[i]);
					i++;
					j++;
				}
				break;
			case '?':
				printf("Unkown option %c\n", optopt);
				return false;
		}
	}

	return true;
}


/*
 * do_cOption() - -c 옵션 처리: 특정 학생의 점수 조회
 *
 * score.csv에서 해당 학생 ID의 행을 찾아 총점(마지막 컬럼)을 출력한다.
 */
void do_cOption(char (*ids)[FILELEN])
{
	FILE *fp;
	char tmp[BUFLEN];
	int i = 0;
	char *p, *saved;

	if((fp = fopen("score.csv", "r")) == NULL){
		fprintf(stderr, "file open error for score.csv\n");
		return;
	}

	fscanf(fp, "%s\n", tmp);  /* 첫 행(헤더) 건너뛰기 */

	/* 각 행을 읽으며 요청된 학생 ID를 찾음 */
	while(fscanf(fp, "%s\n", tmp) != EOF)
	{
		p = strtok(tmp, ",");  /* 첫 컬럼 = 학생 ID */

		if(!is_exist(ids, tmp))
			continue;

		printf("%s's score : ", tmp);

		/* 마지막 컬럼(총점)까지 이동 */
		while((p = strtok(NULL, ",")) != NULL)
			saved = p;

		printf("%s\n", saved);
	}
	fclose(fp);
}

/*
 * is_exist() - 문자열 배열에서 특정 문자열 존재 여부 확인
 *
 * src 배열을 순회하며 target과 일치하는 항목이 있으면 true 반환.
 * ARGNUM 개까지만 검사하며, 빈 문자열을 만나면 종료.
 */
int is_exist(char (*src)[FILELEN], char *target)
{
	int i = 0;

	while(1)
	{
		if(i >= ARGNUM)
			return false;
		else if(!strcmp(src[i], ""))
			return false;
		else if(!strcmp(src[i++], target))
			return true;
	}
	return false;
}

/*
 * set_scoreTable() - 채점표 초기화
 *
 * score_table.csv 파일이 있으면 읽어오고, 없으면 대화형으로 생성 후 저장한다.
 */
void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];

	sprintf(filename, "%s/%s", ansDir, "score_table.csv");

	/* score_table.csv 존재 여부 확인 */
	if(access(filename, F_OK) == 0)
		read_scoreTable(filename);    /* 기존 파일에서 읽기 */
	else{
		make_scoreTable(ansDir);      /* 대화형으로 새로 생성 */
		write_scoreTable(filename);   /* 생성된 채점표를 파일로 저장 */
	}
}

/*
 * read_scoreTable() - CSV 파일에서 채점표 읽기
 *
 * "문제명,점수" 형식의 각 행을 파싱하여 score_table 배열에 저장한다.
 */
void read_scoreTable(char *path)
{
	FILE *fp;
	char qname[FILELEN];
	char score[BUFLEN];
	int idx = 0;

	if((fp = fopen(path, "r")) == NULL){
		fprintf(stderr, "file open error for %s\n", path);
		return ;
	}

	/* 쉼표 구분자로 문제명과 점수를 파싱 */
	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){
		strcpy(score_table[idx].qname, qname);
		score_table[idx++].score = atof(score);
	}

	fclose(fp);
}

/*
 * make_scoreTable() - 대화형 채점표 생성
 *
 * 정답 디렉토리를 탐색하여 문제 목록을 수집하고,
 * 사용자에게 배점을 입력받아 score_table에 저장한다.
 * 생성 방식: 1) 유형별 일괄 배점, 2) 문제별 개별 배점
 */
void make_scoreTable(char *ansDir)
{
	int type, num;
	double score, bscore, pscore;
	struct dirent *dirp, *c_dirp;
	DIR *dp, *c_dp;
	char tmp[BUFLEN];
	int idx = 0;
	int i;

	num = get_create_type();  /* 배점 입력 방식 선택 */

	if(num == 1)
	{
		/* 방식 1: 빈칸/프로그램 유형별 일괄 배점 */
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);
		printf("Input value of program question : ");
		scanf("%lf", &pscore);
	}

	if((dp = opendir(ansDir)) == NULL){
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	}

	/* 정답 디렉토리의 하위 디렉토리를 순회하며 문제 파일 수집 */
	while((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", ansDir, dirp->d_name);

		if((c_dp = opendir(tmp)) == NULL){
			fprintf(stderr, "open dir error for %s\n", tmp);
			return;
		}

		/* 각 문제 디렉토리 내의 파일을 순회 */
		while((c_dirp = readdir(c_dp)) != NULL)
		{
			if(!strcmp(c_dirp->d_name, ".") || !strcmp(c_dirp->d_name, ".."))
				continue;

			if((type = get_file_type(c_dirp->d_name)) < 0)
				continue;

			strcpy(score_table[idx++].qname, c_dirp->d_name);
		}

		closedir(c_dp);
	}

	closedir(dp);
	sort_scoreTable(idx);  /* 문제 번호순으로 정렬 */

	/* 각 문제에 배점 할당 */
	for(i = 0; i < idx; i++)
	{
		type = get_file_type(score_table[i].qname);

		if(num == 1)
		{
			/* 유형별 일괄 배점 */
			if(type == TEXTFILE)
				score = bscore;
			else if(type == CFILE)
				score = pscore;
		}
		else if(num == 2)
		{
			/* 문제별 개별 배점 */
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);
		}

		score_table[i].score = score;
	}
}

/*
 * write_scoreTable() - 채점표를 CSV 파일로 저장
 *
 * score_table의 각 항목을 "문제명,점수" 형식으로 파일에 기록한다.
 */
void write_scoreTable(char *filename)
{
	int fd;
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);

	if((fd = creat(filename, 0666)) < 0){
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for(i = 0; i < num; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		write(fd, tmp, strlen(tmp));
	}

	close(fd);
}


/*
 * set_idTable() - 학생 ID 테이블 초기화
 *
 * 학생 디렉토리를 열어 하위 디렉토리명(=학생 ID)을 id_table에 수집한다.
 * 수집 후 알파벳순으로 정렬한다.
 */
void set_idTable(char *stuDir)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	if((dp = opendir(stuDir)) == NULL){
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		stat(tmp, &statbuf);

		/* 디렉토리만 학생 ID로 인식 (파일은 건너뛰기) */
		if(S_ISDIR(statbuf.st_mode))
			strcpy(id_table[num++], dirp->d_name);
		else
			continue;
	}

	sort_idTable(num);  /* 학생 ID를 알파벳순으로 정렬 */
}

/*
 * sort_idTable() - 학생 ID 테이블을 버블 정렬
 *
 * strcmp로 사전순 비교하여 오름차순 정렬한다.
 */
void sort_idTable(int size)
{
	int i, j;
	char tmp[10];

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

/*
 * sort_scoreTable() - 채점표를 문제 번호순으로 버블 정렬
 *
 * 문제명에서 번호를 추출(get_qname_number)하여 주번호-부번호 순으로 정렬한다.
 * 예: "1-1.txt" < "1-2.txt" < "2-1.c"
 */
void sort_scoreTable(int size)
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){

			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			/* 주번호 우선, 동일하면 부번호로 비교 */
			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

/*
 * get_qname_number() - 문제명에서 주번호와 부번호 추출
 *
 * "1-2.txt" -> num1=1, num2=2
 * "3.c"     -> num1=3, num2=0
 * 구분자: '-' 또는 '.'
 */
void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];

	strncpy(dup, qname, strlen(qname));
	*num1 = atoi(strtok(dup, "-."));  /* 첫 번째 숫자 = 주번호 */

	p = strtok(NULL, "-.");
	if(p == NULL)
		*num2 = 0;       /* 부번호 없음 */
	else
		*num2 = atoi(p);  /* 두 번째 숫자 = 부번호 */
}

/*
 * get_create_type() - 채점표 생성 방식 선택
 *
 * 사용자에게 1(유형별 일괄) 또는 2(문제별 개별) 방식을 선택하게 한다.
 * 올바른 입력이 될 때까지 반복 요청한다.
 */
int get_create_type()
{
	int num;

	while(1)
	{
		printf("score_table.csv file doesn't exist in TREUDIR!\n");
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);

		if(num != 1 && num != 2)
			printf("not correct number!\n");
		else
			break;
	}

	return num;
}

/*
 * [개선 추가] thread_data - 스레드별 채점 데이터 구조체
 *
 * 각 학생의 채점을 별도 스레드에서 수행할 때 사용하는 데이터 컨테이너이다.
 * 스레드 함수(thread_grade)에 인자로 전달되며, 결과도 이 구조체에 저장한다.
 *
 * 필드 설명:
 *   id       - 채점 대상 학생 ID
 *   score    - 해당 학생의 총점 (채점 완료 후 기록)
 *   csv_buf  - 이 학생의 CSV 행 데이터를 누적하는 버퍼
 *              (스레드 간 write() 경합을 피하기 위해 각자 버퍼에 축적)
 *   csv_len  - csv_buf에 현재까지 기록된 바이트 수
 *
 * 설계 이유: 원본은 score_student()에서 매 문제마다 write()를 호출하여
 *           파일 I/O 경합이 발생했다. 개선본은 버퍼에 모아서 한 번에 기록한다.
 */
typedef struct {
	char id[10];
	double score;
	char csv_buf[BUFLEN * 4];
	int csv_len;
} thread_data;


/*
 * [개선 추가] 빈칸 정답 캐시 배열들
 *
 * blank_ans_cache[i]  - i번째 문제의 정답 텍스트를 미리 읽어 저장하는 버퍼
 * blank_ans_len[i]    - i번째 문제의 정답 텍스트 길이
 * blank_ans_loaded[i] - i번째 문제의 캐시 로드 여부 (1=로드됨, 0=미로드)
 *
 * 목적: 빈칸 문제 채점 시 매 학생마다 정답 파일을 다시 open/read/close 하는
 *       반복 I/O를 제거한다. 한 번만 읽어서 메모리에 캐시하면 학생 수만큼의
 *       파일 I/O를 절약할 수 있다.
 */
static char blank_ans_cache[QNUM][BUFLEN * 2];
static int blank_ans_len[QNUM];
static int blank_ans_loaded[QNUM];

/*
 * [개선 추가] load_blank_cache() - 빈칸 정답 텍스트를 메모리에 미리 캐싱
 *
 * score_table에서 TEXTFILE(.txt) 유형인 문제를 찾아 정답 파일을 한 번 읽고
 * blank_ans_cache 배열에 저장한다. 이후 score_blank()에서 캐시된 데이터를
 * 직접 참조하여 파일 I/O 없이 채점한다.
 *
 * 동작 흐름:
 *   1) blank_ans_loaded 배열을 0으로 초기화
 *   2) 각 TEXTFILE 문제에 대해:
 *      a) 파일명에서 확장자를 제거하여 디렉토리명(qname) 생성
 *      b) ansDir/qname/파일명 경로로 정답 파일 열기
 *      c) read()로 전체 내용을 blank_ans_cache[i]에 저장
 *      d) 널 종료 처리 후 길이와 로드 플래그 기록
 */
void load_blank_cache()
{
	int i, fd, len;
	char tmp[BUFLEN];
	char qname[FILELEN];

	memset(blank_ans_loaded, 0, sizeof(blank_ans_loaded));

	for(i = 0; i < QNUM; i++){
		if(score_table[i].score == 0)
			break;  /* 배점이 0이면 문제 목록 끝 */

		/* TEXTFILE이 아닌 문제는 건너뛰기 (프로그램 문제는 캐시 불필요) */
		if(get_file_type(score_table[i].qname) != TEXTFILE)
			continue;

		/* 파일명에서 확장자 제거하여 디렉토리명 생성 (예: "1-1.txt" -> "1-1") */
		memset(qname, 0, sizeof(qname));
		memcpy(qname, score_table[i].qname,
			strlen(score_table[i].qname) - strlen(strrchr(score_table[i].qname, '.')));

		/* 정답 파일 경로 조합 및 읽기 */
		sprintf(tmp, "%s/%s/%s", ansDir, qname, score_table[i].qname);
		fd = open(tmp, O_RDONLY);
		if(fd < 0) continue;  /* 파일 열기 실패 시 건너뛰기 */

		/* 파일 전체를 캐시 버퍼에 읽기 */
		len = read(fd, blank_ans_cache[i], sizeof(blank_ans_cache[0]) - 1);
		close(fd);
		if(len < 0) len = 0;
		blank_ans_cache[i][len] = '\0';  /* 널 종료 보장 */
		blank_ans_len[i] = len;
		blank_ans_loaded[i] = 1;  /* 캐시 로드 완료 표시 */
	}
}

/* wait_batch 함수 전방 선언 (precompile_students에서 사용) */
void wait_batch(pid_t *pids, int count);

/*
 * [개선 추가] precompile_students() - 학생 C 프로그램 사전 컴파일+실행 파이프라인
 *
 * 모든 학생의 모든 C 문제를 fork()로 동시에 컴파일하고, 컴파일 성공 시
 * 같은 자식 프로세스에서 바로 실행까지 수행한다.
 *
 * 왜 필요한가:
 *   원본은 채점 시점에 1명씩 순차적으로 컴파일-실행을 수행하여 매우 느렸다.
 *   개선본은 모든 학생의 컴파일-실행을 미리 병렬로 처리해 둔 뒤,
 *   채점 단계에서는 이미 생성된 결과 파일(.stdout, .stdexe)만 비교하면 된다.
 *
 * 동작 흐름:
 *   1) C 문제 목록 수집 (cfile_indices, cfile_qnames 배열)
 *   2) 학생 x 문제 조합마다:
 *      a) 소스 파일 존재 확인
 *      b) 이미 실행 결과(.stdout)가 있으면 건너뛰기
 *      c) fork()로 자식 프로세스 생성
 *      d) 자식에서: fork()로 gcc 컴파일 -> waitpid()로 완료 대기
 *         -> 성공 시 execv()로 실행, 실패 시 exit(1)
 *   3) 모든 자식 프로세스를 wait_batch()로 대기 (타임아웃 포함)
 *
 * 컴파일+실행을 단일 자식에서 연결하는 이유:
 *   fork() 오버헤드를 줄이고, 컴파일 성공 시 불필요한 IPC 없이
 *   바로 실행으로 이어진다. 실패 시 즉시 exit(1)으로 종료.
 */
void precompile_students()
{
	int s, q, i;
	int num_students = 0;
	int cfile_count = 0;
	int cfile_indices[QNUM];          /* C 문제의 score_table 인덱스 */
	char cfile_qnames[QNUM][FILELEN]; /* C 문제의 확장자 제거된 이름 */
	int cfile_isthread[QNUM];         /* 각 C 문제의 스레드 여부 */
	pid_t pids[SNUM * QNUM];         /* 생성된 자식 프로세스 PID 배열 */
	int pid_count = 0;
	char src[BUFLEN], exe[BUFLEN], err[BUFLEN], out[BUFLEN];
	int errfd, outfd;

	/* 활성 학생 수 카운트 */
	while(num_students < SNUM && strcmp(id_table[num_students], ""))
		num_students++;

	/* C 문제 목록 수집: 파일 유형이 CFILE인 문제만 필터링 */
	for(i = 0; i < QNUM && score_table[i].score != 0; i++){
		if(get_file_type(score_table[i].qname) != CFILE)
			continue;
		cfile_indices[cfile_count] = i;
		/* 확장자(.c) 제거하여 실행파일명/디렉토리명 생성 */
		memset(cfile_qnames[cfile_count], 0, FILELEN);
		memcpy(cfile_qnames[cfile_count], score_table[i].qname,
			strlen(score_table[i].qname) - strlen(strrchr(score_table[i].qname, '.')));
		cfile_isthread[cfile_count] = is_thread(cfile_qnames[cfile_count]);
		cfile_count++;
	}

	/* 모든 학생 x 모든 C 문제 조합에 대해 사전 컴파일+실행 */
	for(s = 0; s < num_students; s++){
		for(q = 0; q < cfile_count; q++){
			/* 학생의 소스 파일 존재 확인 */
			sprintf(src, "%s/%s/%s", stuDir, id_table[s], score_table[cfile_indices[q]].qname);
			if(access(src, F_OK) < 0) continue;  /* 소스 없으면 건너뛰기 */

			/* 이미 실행 결과가 있으면 건너뛰기 (중복 작업 방지) */
			sprintf(out, "%s/%s/%s.stdout", stuDir, id_table[s], cfile_qnames[q]);
			if(access(out, F_OK) == 0) continue;

			/* 실행 파일 및 에러 파일 경로 설정 */
			sprintf(exe, "%s/%s/%s.stdexe", stuDir, id_table[s], cfile_qnames[q]);
			sprintf(err, "%s/%s/%s_error.txt", stuDir, id_table[s], cfile_qnames[q]);

			/* 에러 출력 및 실행 결과 파일 생성 */
			errfd = creat(err, 0666);
			outfd = creat(out, 0666);

			/* 자식 프로세스 생성 (컴파일+실행 담당) */
			pids[pid_count] = fork();
			if(pids[pid_count] < 0){
				/* fork 실패 시: 기존 프로세스들을 모두 대기한 후 재시도 */
				wait_batch(pids, pid_count);
				pid_count = 0;
				pids[pid_count] = fork();
				if(pids[pid_count] < 0){
					close(errfd);
					close(outfd);
					continue;  /* 재시도도 실패하면 이 문제는 건너뛰기 */
				}
			}
			if(pids[pid_count] == 0){
				/*
				 * === 자식 프로세스 영역 ===
				 * 1단계: gcc로 컴파일
				 * 2단계: 컴파일 성공 시 실행
				 */
				pid_t gcc_pid;
				int gcc_status;
				/* -lpthread 포함/미포함 gcc 인자 배열 */
				char *args_t[] = {"gcc", "-o", exe, src, "-lpthread", NULL};
				char *args_n[] = {"gcc", "-o", exe, src, NULL};

				/* 1단계: 손자 프로세스에서 gcc 컴파일 수행 */
				gcc_pid = fork();
				if(gcc_pid == 0){
					dup2(errfd, STDERR);   /* gcc 에러 출력을 파일로 리다이렉트 */
					close(errfd);
					close(outfd);
					if(tOption && cfile_isthread[q])
						execvp("gcc", args_t);  /* 스레드 문제: -lpthread 포함 */
					else
						execvp("gcc", args_n);  /* 일반 문제 */
					exit(1);  /* execvp 실패 시 */
				}
				close(errfd);
				waitpid(gcc_pid, &gcc_status, 0);  /* gcc 완료 대기 */

				/* 컴파일 실패 시 즉시 종료 */
				if(!WIFEXITED(gcc_status) || WEXITSTATUS(gcc_status) != 0){
					close(outfd);
					exit(1);
				}

				/* 2단계: 컴파일 성공 -> 실행 파일 실행 */
				{
					char *args[] = {exe, NULL};
					/* stdin을 /dev/null로 리다이렉트 (입력 대기 방지) */
					int devnull = open("/dev/null", O_RDONLY);
					if(devnull >= 0){ dup2(devnull, 0); close(devnull); }
					dup2(outfd, STDOUT);  /* 실행 결과를 파일로 리다이렉트 */
					close(outfd);
					execv(exe, args);     /* 학생 프로그램 실행 */
					exit(1);              /* execv 실패 시 */
				}
			}
			/* 부모 프로세스: fd 정리 및 카운트 증가 */
			close(errfd);
			close(outfd);
			pid_count++;
		}
	}

	/* 모든 사전 컴파일+실행 프로세스를 대기 (타임아웃 포함) */
	if(pid_count > 0)
		wait_batch(pids, pid_count);
}

/*
 * [개선 추가] wait_batch() - 다수의 자식 프로세스를 타임아웃과 함께 대기
 *
 * VLA(Variable Length Array)로 done 배열을 생성하여 각 프로세스의 완료 여부를 추적한다.
 * WNOHANG 폴링으로 비차단 대기를 수행하며, OVER(5초) 경과 시
 * 아직 완료되지 않은 프로세스에 SIGKILL을 보내 강제 종료한다.
 *
 * 왜 필요한가:
 *   학생 프로그램이 무한 루프에 빠지거나 입력을 기다리면 영원히 블록될 수 있다.
 *   타임아웃으로 시스템 전체가 멈추는 것을 방지한다.
 *
 * 매개변수:
 *   pids  - 대기할 자식 프로세스의 PID 배열
 *   count - PID 배열의 크기
 *
 * 동작 흐름:
 *   1) done[] 배열을 0으로 초기화, remaining = count
 *   2) 루프: 각 프로세스에 대해 waitpid(WNOHANG) 호출
 *      - 완료된 프로세스는 done[i]=1, remaining 감소
 *   3) OVER초 경과 시: 미완료 프로세스에 SIGKILL + waitpid(블로킹) 후 루프 종료
 *   4) 아직 대기 중이면 usleep(5ms)으로 CPU 과부하 방지 후 재시도
 */
void wait_batch(pid_t *pids, int count)
{
	int i, status, remaining;
	int done[count];            /* VLA: 각 프로세스 완료 플래그 */
	time_t batch_start;

	memset(done, 0, sizeof(int) * count);
	batch_start = time(NULL);
	remaining = count;

	while(remaining > 0){
		/* 모든 프로세스를 비차단(WNOHANG)으로 폴링 */
		for(i = 0; i < count; i++){
			if(done[i]) continue;  /* 이미 완료된 프로세스는 건너뛰기 */
			if(waitpid(pids[i], &status, WNOHANG) != 0){
				done[i] = 1;       /* 완료 표시 */
				remaining--;
			}
		}
		/* OVER초 타임아웃 경과 시: 미완료 프로세스 강제 종료 */
		if(remaining > 0 && difftime(time(NULL), batch_start) > OVER){
			for(i = 0; i < count; i++){
				if(!done[i]){
					kill(pids[i], SIGKILL);           /* 강제 종료 시그널 */
					waitpid(pids[i], &status, 0);     /* 좀비 방지를 위한 회수 */
				}
			}
			break;  /* 타임아웃 후 루프 종료 */
		}
		if(remaining > 0) usleep(5000);  /* 5ms 대기로 CPU 과부하 방지 */
	}
}

/*
 * [개선 추가] preprocess_answers() - 정답 C 프로그램 사전 컴파일+실행
 *
 * 정답 디렉토리의 C 프로그램들을 미리 컴파일하고 실행하여
 * .exe(실행파일)과 .stdout(실행 결과) 파일을 생성해 둔다.
 * 이후 채점 시 정답 프로그램을 다시 컴파일/실행할 필요가 없다.
 *
 * 동작 흐름:
 *   1단계: 모든 C 문제의 정답을 병렬 컴파일 (fork/execvp)
 *          - 이미 .exe가 존재하면 건너뛰기
 *          - gcc 에러 출력은 /dev/null로 버림
 *   2단계: 모든 컴파일된 정답을 병렬 실행 (fork/execv)
 *          - 이미 .stdout이 존재하면 건너뛰기
 *          - stdin을 /dev/null로 리다이렉트 (입력 대기 방지)
 *          - alarm(OVER)으로 실행 시간 제한
 */
void preprocess_answers()
{
	int i, type, fd;
	char tmp[BUFLEN];
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	int isthread;
	pid_t pids[QNUM];
	int pid_count, status;
	char qnames[QNUM][FILELEN];  /* 확장자 제거된 문제명 배열 */
	int qindices[QNUM];          /* score_table 인덱스 매핑 */
	int qcount = 0;

	/* C 문제 목록 수집 */
	for(i = 0; i < QNUM; i++){
		if(score_table[i].score == 0)
			break;

		type = get_file_type(score_table[i].qname);
		if(type != CFILE)
			continue;

		/* 확장자 제거하여 디렉토리명/실행파일명 생성 */
		memset(qnames[qcount], 0, sizeof(qnames[qcount]));
		memcpy(qnames[qcount], score_table[i].qname,
			strlen(score_table[i].qname) - strlen(strrchr(score_table[i].qname, '.')));
		qindices[qcount] = i;
		qcount++;
	}

	/* 1단계: 병렬 컴파일 */
	pid_count = 0;
	for(i = 0; i < qcount; i++){
		isthread = is_thread(qnames[i]);

		sprintf(tmp_e, "%s/%s/%s.exe", ansDir, qnames[i], qnames[i]);
		/* 이미 실행 파일이 존재하면 컴파일 건너뛰기 */
		if(access(tmp_e, F_OK) < 0){
			sprintf(tmp_f, "%s/%s/%s", ansDir, qnames[i], score_table[qindices[i]].qname);

			pids[pid_count] = fork();
			if(pids[pid_count] == 0){
				/* 자식: gcc 컴파일 수행 */
				char *args_t[] = {"gcc", "-o", tmp_e, tmp_f, "-lpthread", NULL};
				char *args_n[] = {"gcc", "-o", tmp_e, tmp_f, NULL};
				fd = open("/dev/null", O_WRONLY);
				dup2(fd, STDERR);  /* 에러 출력 무시 */
				close(fd);
				if(tOption && isthread)
					execvp("gcc", args_t);
				else
					execvp("gcc", args_n);
				exit(1);
			}
			pid_count++;
		}
	}

	/* 모든 컴파일 프로세스 대기 */
	for(i = 0; i < pid_count; i++)
		waitpid(pids[i], &status, 0);

	/* 2단계: 병렬 실행 */
	pid_count = 0;
	for(i = 0; i < qcount; i++){
		sprintf(tmp, "%s/%s/%s.stdout", ansDir, qnames[i], qnames[i]);
		/* 이미 실행 결과가 존재하면 건너뛰기 */
		if(access(tmp, F_OK) < 0){
			fd = creat(tmp, 0666);  /* 실행 결과 저장 파일 생성 */
			sprintf(tmp_e, "%s/%s/%s.exe", ansDir, qnames[i], qnames[i]);

			pids[pid_count] = fork();
			if(pids[pid_count] == 0){
				/* 자식: 정답 프로그램 실행 */
				char *args[] = {tmp_e, NULL};
				/* stdin을 /dev/null로 리다이렉트 (입력 대기 방지) */
				int devnull = open("/dev/null", O_RDONLY);
				if(devnull >= 0){ dup2(devnull, 0); close(devnull); }
				alarm(OVER);       /* OVER초 후 SIGALRM으로 자동 종료 */
				dup2(fd, STDOUT);  /* 실행 결과를 파일로 리다이렉트 */
				close(fd);
				execv(tmp_e, args);
				exit(1);
			}
			close(fd);
			pid_count++;
		}
	}

	/* 모든 실행 프로세스 대기 */
	for(i = 0; i < pid_count; i++)
		waitpid(pids[i], &status, 0);
}

/*
 * [개선 추가] thread_grade() - 스레드 채점 함수
 *
 * 개별 학생의 모든 문제를 채점하는 스레드 워커 함수이다.
 * pthread_create()로 생성된 스레드에서 실행된다.
 *
 * 원본의 score_student() 함수와 동일한 로직이지만, 핵심적인 차이점:
 *   1) 파일에 직접 write() 하지 않고 csv_buf에 버퍼링
 *      -> 스레드 간 write() 경합(race condition) 방지
 *   2) 결과를 thread_data 구조체에 저장하여 메인 스레드가 수집
 *
 * 매개변수: arg - thread_data 구조체 포인터
 * 반환: NULL (결과는 thread_data에 저장됨)
 *
 * 동작 흐름:
 *   1) thread_data에서 학생 ID 확인
 *   2) csv_buf에 "학생ID," 기록
 *   3) 각 문제에 대해:
 *      a) 학생 답안 파일 존재 여부 확인
 *      b) TEXTFILE -> score_blank(), CFILE -> score_program() 호출
 *      c) 결과(0, 배점, 감점)를 csv_buf에 기록
 *   4) 총점을 csv_buf 마지막에 기록
 */
void *thread_grade(void *arg)
{
	thread_data *td = (thread_data *)arg;
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	/* CSV 버퍼 초기화 및 학생 ID 기록 */
	td->csv_len = 0;
	td->csv_len += sprintf(td->csv_buf + td->csv_len, "%s,", td->id);

	for(i = 0; i < size; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, td->id, score_table[i].qname);

		/* 학생 답안 파일이 존재하는지 확인 */
		if(access(tmp, F_OK) < 0)
			result = false;  /* 파일 없음 = 0점 */
		else
		{
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;  /* 지원하지 않는 파일 유형 건너뛰기 */

			if(type == TEXTFILE)
				result = score_blank(td->id, score_table[i].qname);
			else if(type == CFILE)
				result = score_program(td->id, score_table[i].qname);
		}

		/* 결과에 따라 CSV에 점수 기록 */
		if(result == false)
			td->csv_len += sprintf(td->csv_buf + td->csv_len, "0,");
		else{
			if(result == true){
				/* 만점: 배점 전체 부여 */
				score += score_table[i].score;
				td->csv_len += sprintf(td->csv_buf + td->csv_len, "%.2f,", score_table[i].score);
			}
			else if(result < 0){
				/* 감점: 배점 + 감점(음수) = 실제 점수 */
				score = score + score_table[i].score + result;
				td->csv_len += sprintf(td->csv_buf + td->csv_len, "%.2f,", score_table[i].score + result);
			}
		}
	}

	/* 총점을 CSV 행 마지막에 기록 */
	td->csv_len += sprintf(td->csv_buf + td->csv_len, "%.2f\n", score);
	td->score = score;

	return NULL;
}

/*
 * [개선] score_students() - 전체 학생 채점 함수 (순차 -> 병렬 재설계)
 *
 * 원본: 학생 한 명씩 순차적으로 score_student()를 호출
 * 개선: 4단계 병렬 파이프라인으로 재설계
 *
 * 4단계 파이프라인:
 *   1단계: score.csv 파일 생성 및 헤더 행 작성
 *   2단계: 사전 처리 (fork로 병렬 수행)
 *          - 자식 프로세스: preprocess_answers() (정답 컴파일+실행)
 *          - 부모 프로세스: load_blank_cache() + precompile_students()
 *            (빈칸 캐시 로드 + 학생 프로그램 사전 컴파일)
 *          -> 정답 처리와 학생 처리를 동시에 수행하여 시간 절약
 *   3단계: 학생별 스레드 생성 -> 병렬 채점 (thread_grade)
 *          -> 각 스레드가 독립적으로 자신의 학생을 채점
 *   4단계: 모든 스레드 join 후 결과 수집 및 CSV 기록
 *          -> 스레드별 csv_buf를 순서대로 파일에 기록
 *
 * 이 설계의 장점:
 *   - 사전 컴파일로 채점 시 gcc 호출이 대부분 불필요해짐
 *   - 스레드 병렬로 빈칸 채점이 동시에 진행됨
 *   - 버퍼링으로 파일 I/O 횟수가 대폭 감소
 */
void score_students()
{
	double score = 0;
	int num;
	int fd;
	int size = sizeof(id_table) / sizeof(id_table[0]);
	int count = 0;
	pthread_t threads[SNUM];     /* 학생별 스레드 핸들 */
	thread_data td[SNUM];        /* 학생별 채점 데이터 */

	/* 활성 학생 수 카운트 */
	for(num = 0; num < size; num++){
		if(!strcmp(id_table[num], ""))
			break;
		count++;
	}

	/* 1단계: score.csv 파일 생성 및 헤더 행 작성 */
	if((fd = creat("score.csv", 0666)) < 0){
		fprintf(stderr, "creat error for score.csv");
		return;
	}
	write_first_row(fd);

	/*
	 * 2단계: 사전 처리 (정답 처리와 학생 처리를 동시에 수행)
	 *
	 * fork()로 자식을 만들어 정답 사전 처리를 맡기고,
	 * 부모는 빈칸 캐시 로드 + 학생 프로그램 사전 컴파일을 수행한다.
	 * waitpid()로 자식이 끝날 때까지 대기하여 동기화한다.
	 */
	{
		pid_t ans_pid;
		int ans_status;
		ans_pid = fork();
		if(ans_pid == 0){
			preprocess_answers();  /* 자식: 정답 컴파일+실행 */
			_exit(0);              /* _exit: 버퍼 플러시 없이 종료 (부모와 충돌 방지) */
		}
		load_blank_cache();        /* 부모: 빈칸 정답 캐시 로드 */
		precompile_students();     /* 부모: 학생 프로그램 사전 컴파일+실행 */
		waitpid(ans_pid, &ans_status, 0);  /* 자식 완료 대기 */
	}

	/* 3단계: 학생별 스레드 생성 -> 병렬 채점 */
	for(num = 0; num < count; num++){
		strcpy(td[num].id, id_table[num]);
		pthread_create(&threads[num], NULL, thread_grade, &td[num]);
	}

	/* 모든 채점 스레드 완료 대기 */
	for(num = 0; num < count; num++)
		pthread_join(threads[num], NULL);

	/* 4단계: 결과 수집 및 CSV 기록 */
	for(num = 0; num < count; num++){
		if(pOption)
			printf("%s is finished.. score : %.2f\n", td[num].id, td[num].score);
		else
			printf("%s is finished..\n", td[num].id);

		/* 각 스레드가 축적한 CSV 버퍼를 파일에 순서대로 기록 */
		write(fd, td[num].csv_buf, td[num].csv_len);
		score += td[num].score;
	}

	if(pOption)
		printf("Total average : %.2f\n", score / count);

	close(fd);
}

/*
 * [원본 보존] score_student() - 개별 학생 순차 채점 함수
 *
 * 원본의 순차 채점 방식을 보존한 함수이다.
 * 개선본에서는 thread_grade()로 대체되어 직접 호출되지 않지만,
 * 원본 로직을 참조하거나 디버깅용으로 유지한다.
 *
 * 차이점: 이 함수는 매 문제마다 직접 write()를 호출하여
 *         파일에 바로 기록하는 방식이다 (스레드 미사용).
 */
double score_student(int fd, char *id)
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	for(i = 0; i < size ; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);

		if(access(tmp, F_OK) < 0)
			result = false;
		else
		{
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;

			if(type == TEXTFILE)
				result = score_blank(id, score_table[i].qname);
			else if(type == CFILE)
				result = score_program(id, score_table[i].qname);
		}

		if(result == false)
			write(fd, "0,", 2);
		else{
			if(result == true){
				score += score_table[i].score;
				sprintf(tmp, "%.2f,", score_table[i].score);
			}
			else if(result < 0){
				score = score + score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			write(fd, tmp, strlen(tmp));
		}
	}

	if(pOption)
		printf("%s is finished.. score : %.2f\n", id, score);
	else
		printf("%s is finished..\n", id);

	sprintf(tmp, "%.2f\n", score);
	write(fd, tmp, strlen(tmp));

	return score;
}

/*
 * [개선] write_first_row() - CSV 헤더 행 작성
 *
 * 개선 내용: 원본은 매 컬럼마다 write()를 호출하여 시스템 콜 오버헤드가 컸다.
 *           개선본은 buf에 전체 헤더를 sprintf로 축적한 뒤 단일 write()로 기록한다.
 *
 * CSV 헤더 형식: ",문제1,문제2,...,문제N,sum\n"
 * 첫 번째 쉼표는 학생 ID 열(빈 헤더)을 나타낸다.
 */
void write_first_row(int fd)
{
	int i;
	char buf[BUFLEN * 4];  /* 헤더 축적 버퍼 */
	int len = 0;
	int size = sizeof(score_table) / sizeof(score_table[0]);

	buf[len++] = ',';  /* 첫 컬럼(학생 ID)의 빈 헤더 */

	/* 각 문제명을 쉼표 구분으로 추가 */
	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)
			break;
		len += sprintf(buf + len, "%s,", score_table[i].qname);
	}
	len += sprintf(buf + len, "sum\n");  /* 마지막 컬럼: 총점 */
	write(fd, buf, len);  /* 단일 write()로 전체 헤더 기록 */
}

/*
 * [개선] get_answer() - 정답 파일에서 답안 문자열 읽기
 *
 * 개선 내용: 원본은 한 바이트씩 read()를 호출하여 매우 느렸다.
 *           개선본은 BUFLEN 크기 블록 단위로 한 번에 읽은 뒤,
 *           ':' 구분자까지 result에 복사하고, 남은 데이터가 있으면
 *           lseek()로 파일 포인터를 되감아 다음 호출에서 이어 읽을 수 있게 한다.
 *
 * 정답 파일 형식: "답안1:답안2:답안3" (콜론으로 구분된 복수 정답)
 * 이 함수는 한 번 호출 시 하나의 답안을 반환한다.
 *
 * 매개변수:
 *   fd     - 열린 정답 파일 디스크립터
 *   result - 읽은 답안 문자열을 저장할 버퍼
 * 반환: result 포인터
 */
char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;
	char buf[BUFLEN];
	int n, i;

	memset(result, 0, BUFLEN);

	/* 블록 단위 읽기: 한 번의 read()로 최대 BUFLEN-1 바이트 */
	n = read(fd, buf, BUFLEN - 1);
	if(n <= 0)
		return result;  /* 파일 끝 또는 에러 */

	/* ':' 구분자를 찾아 그 전까지를 result에 복사 */
	for(i = 0; i < n; i++){
		if(buf[i] == ':'){
			i++;  /* ':' 다음 위치로 이동 */
			break;
		}
		result[idx++] = buf[i];
	}

	/* 아직 읽지 않은 데이터가 남아있으면 lseek로 파일 포인터를 되감기 */
	if(i < n)
		lseek(fd, -(n - i), SEEK_CUR);

	/* 마지막 문자가 개행이면 제거 */
	if(idx > 0 && result[idx - 1] == '\n')
		result[idx - 1] = '\0';

	return result;
}

/*
 * [개선] score_blank() - 빈칸 채우기 문제 채점
 *
 * 개선 내용:
 *   1) 캐시 활용: blank_ans_cache에서 정답 텍스트를 가져와 파일 I/O 제거
 *   2) 문자열 직접 비교 단축: strcmp로 정답과 일치하면 즉시 true 반환
 *      (트리 비교 없이 빠르게 판정)
 *   3) 캐시 미스 시 폴백: 파일에서 직접 읽기
 *
 * 동작 흐름:
 *   1) 학생 답안 파일 열기 및 답안 읽기
 *   2) 빈 답안, 괄호 불일치 검사
 *   3) 세미콜론 처리 (있으면 제거하고 플래그 기록)
 *   4) 정답 텍스트 획득 (캐시 우선, 없으면 파일에서 읽기)
 *   5) 학생 답안을 토큰화하여 구문 트리 생성
 *   6) 각 정답(':'로 구분된 복수 정답)에 대해:
 *      a) 세미콜론 일관성 검사
 *      b) 문자열 직접 비교 -> 일치 시 즉시 true
 *      c) 정답 토큰화 -> 구문 트리 생성 -> 트리 비교
 *   7) 하나라도 일치하면 true, 모두 불일치면 false
 */
int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char ans_buf[BUFLEN * 2];   /* 정답 텍스트 전체를 담는 버퍼 */
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;
	int ans_len, alen;
	char *apos, *colon;

	/* 파일명에서 확장자 제거하여 디렉토리명 생성 */
	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	/* 학생 답안 파일 열기 및 읽기 */
	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	fd_std = open(tmp, O_RDONLY);
	strcpy(s_answer, get_answer(fd_std, s_answer));
	close(fd_std);

	/* 빈 답안 검사 */
	if(!strcmp(s_answer, ""))
		return false;

	/* 괄호 짝 검사 */
	if(!check_brackets(s_answer))
		return false;

	/* 좌우 공백 제거 */
	strcpy(s_answer, ltrim(rtrim(s_answer)));

	/* 세미콜론 처리: 마지막 문자가 ';'이면 제거하고 플래그 기록 */
	if(s_answer[strlen(s_answer) - 1] == ';'){
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	}

	/*
	 * [개선] 캐시 활용 정답 획득
	 * blank_ans_cache에서 해당 문제의 정답을 찾아 ans_buf에 복사.
	 * 캐시에 없으면 파일에서 직접 읽기 (폴백).
	 */
	{
		int ci;
		/* 캐시에서 해당 문제 찾기 */
		for(ci = 0; ci < QNUM; ci++){
			if(blank_ans_loaded[ci] && !strcmp(score_table[ci].qname, filename))
				break;
		}
		if(ci < QNUM){
			/* 캐시 히트: 메모리에서 직접 복사 (파일 I/O 불필요) */
			memcpy(ans_buf, blank_ans_cache[ci], blank_ans_len[ci] + 1);
			ans_len = blank_ans_len[ci];
		} else {
			/* 캐시 미스: 파일에서 읽기 (폴백) */
			sprintf(tmp, "%s/%s/%s", ansDir, qname, filename);
			fd_ans = open(tmp, O_RDONLY);
			ans_len = read(fd_ans, ans_buf, sizeof(ans_buf) - 1);
			close(fd_ans);
			if(ans_len < 0) ans_len = 0;
			ans_buf[ans_len] = '\0';
		}
	}

	/* 학생 답안을 토큰화 */
	if(!make_tokens(s_answer, tokens))
		return false;

	/* 학생 답안의 구문 트리 생성 */
	idx = 0;
	std_root = make_tree(std_root, tokens, &idx, 0);

	/* 정답 텍스트를 ':' 구분자로 분리하여 각 정답과 비교 */
	apos = ans_buf;
	while(*apos)
	{
		ans_root = NULL;
		result = true;

		/* ':' 구분자로 하나의 정답을 추출 */
		colon = strchr(apos, ':');
		if(colon){
			alen = colon - apos;
			memcpy(a_answer, apos, alen);
			a_answer[alen] = '\0';
			apos = colon + 1;  /* 다음 정답으로 이동 */
		} else {
			/* 마지막 정답 (더 이상 ':' 없음) */
			alen = strlen(apos);
			memcpy(a_answer, apos, alen + 1);
			apos += alen;
		}

		/* 끝의 개행 제거 */
		if(alen > 0 && a_answer[alen - 1] == '\n')
			a_answer[--alen] = '\0';

		if(alen == 0)
			break;  /* 빈 정답이면 종료 */

		/* 정답 좌우 공백 제거 */
		strcpy(a_answer, ltrim(rtrim(a_answer)));

		/* 세미콜론 일관성 검사 */
		if(has_semicolon == false){
			/* 학생이 세미콜론 없이 답했는데 정답에 세미콜론이 있으면 건너뛰기 */
			if(a_answer[strlen(a_answer) -1] == ';')
				continue;
		}

		else if(has_semicolon == true)
		{
			/* 학생이 세미콜론을 붙였는데 정답에 세미콜론이 없으면 건너뛰기 */
			if(a_answer[strlen(a_answer) - 1] != ';')
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';  /* 비교를 위해 세미콜론 제거 */
		}

		/*
		 * [개선] 문자열 직접 비교 단축
		 * strcmp로 학생 답안과 정답이 문자열 수준에서 동일하면
		 * 트리 비교 없이 즉시 true 반환하여 성능을 향상시킨다.
		 */
		if(!strcmp(s_answer, a_answer)){
			if(std_root != NULL)
				free_node(std_root);
			return true;
		}

		/* 정답을 토큰화하여 구문 트리 생성 */
		if(!make_tokens(a_answer, tokens))
			continue;  /* 토큰화 실패 시 다음 정답 시도 */

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0);

		/* 구문 트리 비교 (교환 법칙, 관계 연산자 반전 등 고려) */
		compare_tree(std_root, ans_root, &result);

		if(result == true){
			if(std_root != NULL)
				free_node(std_root);
			if(ans_root != NULL)
				free_node(ans_root);
			return true;  /* 트리 비교 성공 */
		}

		if(ans_root != NULL)
			free_node(ans_root);
	}

	if(std_root != NULL)
		free_node(std_root);

	return false;  /* 모든 정답과 불일치 */
}

/*
 * score_program() - 프로그램 문제 채점
 *
 * 컴파일 -> 실행 -> 결과 비교의 3단계로 구성된다.
 *
 * 반환값:
 *   false(0) - 컴파일 에러 또는 실행 결과 불일치
 *   true(1)  - 정답 일치
 *   음수     - 경고로 인한 감점
 */
double score_program(char *id, char *filename)
{
	double compile;
	int result;

	compile = compile_program(id, filename);

	/* 컴파일 에러 또는 실패 시 0점 */
	if(compile == ERROR || compile == false)
		return false;

	result = execute_program(id, filename);

	if(!result)
		return false;  /* 실행 결과 불일치 */

	if(compile < 0)
		return compile;  /* 경고 감점 반환 */

	return true;  /* 정답 일치 */
}

/*
 * is_thread() - -t 옵션에 지정된 스레드 문제인지 확인
 *
 * threadFiles 배열을 순회하여 주어진 문제명과 일치하는 항목이 있으면 true 반환.
 * -lpthread 플래그로 컴파일할 문제를 식별하는 데 사용.
 */
int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname))
			return true;
	}
	return false;
}

/*
 * [개선] compile_program() - 학생 프로그램 컴파일 및 에러/경고 검사
 *
 * 개선 내용: 사전 컴파일 캐시 활용
 *   precompile_students()에서 이미 생성한 .stdexe(실행파일)와
 *   _error.txt(에러 파일)이 존재하면 해당 파일을 직접 활용하여
 *   gcc를 다시 호출하지 않는다.
 *
 * 동작 흐름:
 *   1) 정답 프로그램 컴파일 확인 (.exe 존재 확인 -> 없으면 컴파일)
 *   2) 학생 프로그램 사전 컴파일 캐시 확인 (.stdexe 또는 _error.txt)
 *      a) 캐시 히트: 에러 파일 크기 확인 -> 에러 유무 판정
 *      b) 캐시 미스: redirection()으로 gcc 컴파일 수행
 *   3) 에러 파일 분석 (check_error_warning):
 *      - 에러 발견 시 ERROR(0) 반환
 *      - 경고만 있으면 음수 감점값 반환
 *   4) -e 옵션 시 에러 파일을 errorDir로 이동
 *
 * 반환값:
 *   true(1) - 컴파일 성공, 에러/경고 없음
 *   false(0) - 컴파일 실패
 *   ERROR(0) - 에러 발견
 *   음수 - 경고 감점값
 */
double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	/* 파일명에서 확장자 제거 */
	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	isthread = is_thread(qname);

	/* ---- 정답 프로그램 컴파일 확인 ---- */
	sprintf(tmp_e, "%s/%s/%s.exe", ansDir, qname, qname);

	/* 정답 실행 파일이 없으면 컴파일 (preprocess_answers 미완료 시 폴백) */
	if(access(tmp_e, F_OK) < 0){
		sprintf(tmp_f, "%s/%s/%s", ansDir, qname, filename);

		if(tOption && isthread)
			sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
		else
			sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

		sprintf(tmp_e, "%s/%s/%s_error.txt", ansDir, qname, qname);
		fd = creat(tmp_e, 0666);

		redirection(command, fd, STDERR);
		size = lseek(fd, 0, SEEK_END);
		close(fd);
		unlink(tmp_e);

		if(size > 0)
			return false;  /* 정답 컴파일 실패 = 문제 자체의 오류 */
	}

	/* ---- 학생 프로그램 컴파일 (사전 컴파일 캐시 활용) ---- */
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);
	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);

	/*
	 * [개선] 사전 컴파일 캐시 확인
	 * precompile_students()에서 생성한 .stdexe 또는 _error.txt가 존재하면
	 * gcc를 다시 호출하지 않고 기존 결과를 활용한다.
	 */
	{
		int exe_exists = (access(tmp_e, F_OK) == 0);   /* 실행 파일 존재? */
		int err_exists = (access(tmp_f, F_OK) == 0);   /* 에러 파일 존재? */
		if(exe_exists || err_exists){
			/* 에러 파일이 있으면 크기 확인 */
			if(err_exists){
				fd = open(tmp_f, O_RDONLY);
				size = (fd >= 0) ? lseek(fd, 0, SEEK_END) : 0;
				if(fd >= 0) close(fd);
			} else
				size = 0;

			/* 에러 파일에 내용이 있으면 에러/경고 분석 */
			if(size > 0){
				if(eOption){
					/* -e 옵션: 에러 파일을 errorDir로 이동 */
					sprintf(tmp_e, "%s/%s", errorDir, id);
					if(access(tmp_e, F_OK) < 0) mkdir(tmp_e, 0755);
					sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
					rename(tmp_f, tmp_e);
					result = check_error_warning(tmp_e);
				} else {
					result = check_error_warning(tmp_f);
					unlink(tmp_f);
				}
				return result;
			}
			/* 에러 없음: 에러 파일 정리 후 결과 반환 */
			if(err_exists) unlink(tmp_f);
			return exe_exists ? true : false;
		}
	}

	/* ---- 캐시 미스: 직접 gcc 컴파일 수행 (폴백) ---- */
	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	fd = creat(tmp_f, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if(size > 0){
		if(eOption)
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			if(access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			rename(tmp_f, tmp_e);

			result = check_error_warning(tmp_e);
		}
		else{
			result = check_error_warning(tmp_f);
			unlink(tmp_f);
		}

		return result;
	}

	unlink(tmp_f);
	return true;
}

/*
 * [개선] check_error_warning() - 컴파일 에러/경고 파일 분석
 *
 * 개선 내용: 원본에서 fclose() 호출이 누락된 경로가 있었으나,
 *           개선본에서는 모든 경로에서 반드시 fclose()를 호출하도록 수정했다.
 *           "error:" 발견 시 fclose(fp) 후 ERROR 반환, 루프 정상 종료 시에도 fclose(fp) 호출.
 *
 * 동작: 에러 파일을 읽어 "error:"와 "warning:" 문자열을 검색한다.
 *       error 발견 시 ERROR(0) 반환, warning은 개수만큼 감점 누적.
 *
 * 반환값:
 *   ERROR(0) - 에러 발견 (0점 처리)
 *   0.0      - 에러/경고 없음
 *   음수     - 경고 감점 합계 (WARNING * 경고 수)
 */
double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while(fscanf(fp, "%s", tmp) > 0){
		if(!strcmp(tmp, "error:")){
			fclose(fp);  /* [개선] 에러 경로에서도 반드시 fclose 호출 */
			return ERROR;
		}
		else if(!strcmp(tmp, "warning:"))
			warning += WARNING;  /* 경고 1개당 WARNING(-0.1) 감점 */
	}

	fclose(fp);  /* 정상 종료 시 fclose */
	return warning;
}

/*
 * [개선] execute_program() - 학생 프로그램 실행 및 결과 비교
 *
 * 개선 내용:
 *   1) 부모측 SIGKILL 타임아웃: 원본의 inBackground() 방식 대신
 *      WNOHANG 폴링 + SIGKILL로 안정적인 타임아웃 처리
 *   2) /dev/null stdin 리다이렉트: 학생 프로그램이 stdin 입력을 기다리면
 *      즉시 EOF를 받아 블록되지 않도록 함
 *   3) 사전 실행 결과 활용: .stdout 파일이 이미 존재하면 바로 비교
 *
 * 동작 흐름:
 *   1) 정답 실행 결과 확인 (.stdout 없으면 생성)
 *   2) 학생 실행 결과가 이미 있으면 바로 비교
 *   3) 없으면 fork+execv로 학생 프로그램 실행
 *   4) 부모: WNOHANG 폴링으로 대기하며 OVER초 초과 시 SIGKILL
 *   5) compare_resultfile()로 결과 비교
 */
int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;
	int status;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	/* 정답 실행 결과 파일 경로 */
	sprintf(ans_fname, "%s/%s/%s.stdout", ansDir, qname, qname);

	/* 정답 실행 결과가 없으면 생성 (preprocess_answers 미완료 시 폴백) */
	if(access(ans_fname, F_OK) < 0){
		fd = creat(ans_fname, 0666);
		sprintf(tmp, "%s/%s/%s.exe", ansDir, qname, qname);
		redirection(tmp, fd, STDOUT);
		close(fd);
	}

	/* 학생 실행 결과 파일 경로 */
	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);

	/* [개선] 사전 실행 결과가 있으면 바로 비교 (precompile_students에서 생성) */
	if(access(std_fname, F_OK) == 0)
		return compare_resultfile(std_fname, ans_fname);

	/* 사전 실행 결과 없음: 직접 실행 (폴백) */
	fd = creat(std_fname, 0666);

	sprintf(tmp, "%s/%s/%s.stdexe", stuDir, id, qname);

	start = time(NULL);
	pid = fork();

	if(pid == 0){
		/* 자식: 학생 프로그램 실행 */
		char *args[] = {tmp, NULL};
		/* [개선] /dev/null stdin 리다이렉트: 입력 대기 방지 */
		int devnull = open("/dev/null", O_RDONLY);
		if(devnull >= 0){ dup2(devnull, 0); close(devnull); }
		dup2(fd, STDOUT);  /* 실행 결과를 파일로 리다이렉트 */
		close(fd);
		execv(tmp, args);
		exit(1);
	}
	else if(pid > 0){
		/* 부모: 자식 완료를 WNOHANG 폴링으로 대기 */
		close(fd);

		/*
		 * [개선] WNOHANG 폴링 + SIGKILL 타임아웃
		 * 원본은 alarm()/inBackground()를 사용했으나, 이 방식은
		 * 시그널 핸들러 충돌과 경쟁 조건 위험이 있었다.
		 * 개선: 부모가 직접 폴링하며 타임아웃 시 SIGKILL로 강제 종료.
		 */
		while(waitpid(pid, &status, WNOHANG) == 0){
			end = time(NULL);
			if(difftime(end, start) > OVER){
				kill(pid, SIGKILL);             /* 타임아웃: 강제 종료 */
				waitpid(pid, &status, 0);       /* 좀비 방지를 위한 회수 */
				return false;
			}
			usleep(10000);  /* 10ms 대기로 CPU 과부하 방지 */
		}

		/* 시그널로 종료된 경우 (비정상 종료) */
		if(WIFSIGNALED(status)){
			return false;
		}
	}

	/* 실행 결과 파일을 정답과 비교 */
	return compare_resultfile(std_fname, ans_fname);
}

/*
 * [원본 보존] inBackground() - 백그라운드 프로세스 확인 함수
 *
 * 원본에서 실행 중인 프로그램을 "ps | grep" 명령으로 확인하는 함수였다.
 * 개선본에서는 부모측 WNOHANG + SIGKILL 방식으로 타임아웃을 처리하므로
 * 이 함수는 더 이상 사용되지 않지만, 원본 코드를 보존한다.
 *
 * 동작: "ps | grep <name>" 결과를 파일에 기록하고 PID를 추출하여 반환.
 */
pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;

	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	sprintf(command, "ps | grep %s", name);
	redirection(command, fd, STDOUT);

	lseek(fd, 0, SEEK_SET);
	read(fd, tmp, sizeof(tmp));

	if(!strcmp(tmp, "")){
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt");
	return pid;
}

/*
 * [개선] compare_resultfile() - 실행 결과 파일 비교
 *
 * 개선 내용: 원본은 1바이트씩 read()를 호출하여 극도로 느렸다.
 *           개선본은 BUFLEN(1024바이트) 크기 블록 단위로 읽어
 *           버퍼 내에서 인덱스로 탐색한다. 시스템 콜 횟수가 1/1024로 감소.
 *
 * 비교 규칙: 공백(' ')은 무시하고, 대소문자 구분 없이 비교한다.
 * 양쪽 파일을 동시에 공백을 건너뛰며 읽어 각 문자를 비교한다.
 *
 * 반환: true(1) 일치, false(0) 불일치
 */
int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;
	char c1, c2;
	int len1 = 0, len2 = 0;    /* 각 버퍼에 읽힌 데이터 크기 */
	char buf1[BUFLEN], buf2[BUFLEN];  /* 블록 읽기 버퍼 */
	int i1 = 0, i2 = 0;        /* 각 버퍼 내 현재 읽기 위치 */

	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);

	while(1)
	{
		/* file1에서 공백이 아닌 다음 문자 찾기 */
		while(1){
			if(i1 >= len1){
				/* 버퍼 소진 시 블록 단위로 다시 읽기 */
				len1 = read(fd1, buf1, BUFLEN);
				if(len1 <= 0){ len1 = 0; break; }
				i1 = 0;
			}
			if(buf1[i1] != ' ') break;  /* 공백이 아니면 중단 */
			i1++;
		}

		/* file2에서 공백이 아닌 다음 문자 찾기 */
		while(1){
			if(i2 >= len2){
				len2 = read(fd2, buf2, BUFLEN);
				if(len2 <= 0){ len2 = 0; break; }
				i2 = 0;
			}
			if(buf2[i2] != ' ') break;
			i2++;
		}

		/* 양쪽 모두 데이터 소진 시 일치 */
		if(i1 >= len1 && i2 >= len2)
			break;

		/* 현재 문자 추출 (한쪽이 먼저 끝나면 0) */
		c1 = (i1 < len1) ? buf1[i1++] : 0;
		c2 = (i2 < len2) ? buf2[i2++] : 0;

		/* 대소문자 무시 비교 */
		to_lower_case(&c1);
		to_lower_case(&c2);

		if(c1 != c2){
			close(fd1);
			close(fd2);
			return false;  /* 불일치 */
		}
	}
	close(fd1);
	close(fd2);
	return true;  /* 완전 일치 */
}

/*
 * [개선] redirection() - 명령 실행 + 출력 리다이렉션
 *
 * 개선 내용:
 *   1) system() -> fork/execvp 전환: system()은 내부적으로 쉘을 호출하여
 *      멀티스레드 환경에서 안전하지 않다. fork/execvp는 스레드 안전하다.
 *   2) strtok -> strtok_r 전환: strtok는 정적 버퍼를 사용하여
 *      멀티스레드 환경에서 경쟁 조건이 발생한다.
 *      strtok_r은 saveptr 매개변수로 상태를 분리하여 스레드 안전하다.
 *
 * 동작:
 *   1) 명령 문자열을 공백으로 토큰화하여 args[] 배열 생성
 *   2) fork()로 자식 생성
 *   3) 자식: dup2()로 출력 리다이렉트 후 execvp()로 명령 실행
 *   4) 부모: waitpid()로 자식 완료 대기
 */
void redirection(char *command, int new, int old)
{
	pid_t pid;
	int status;
	char cmd_copy[BUFLEN];    /* 원본 command를 보존하기 위한 복사본 */
	char *args[20];           /* 명령어 + 인자 배열 (최대 19개 인자) */
	int argc = 0;
	char *token;
	char *saveptr;            /* strtok_r용 상태 저장 포인터 */

	/* 명령 문자열을 복사본에 복사 (strtok_r이 원본을 변경하므로) */
	strcpy(cmd_copy, command);
	/* [개선] strtok_r: 스레드 안전한 토큰화 */
	token = strtok_r(cmd_copy, " ", &saveptr);
	while(token != NULL && argc < 19){
		args[argc++] = token;
		token = strtok_r(NULL, " ", &saveptr);
	}
	args[argc] = NULL;  /* execvp에 필요한 NULL 종료 */

	pid = fork();

	if(pid < 0){
		fprintf(stderr, "fork error\n");
		return;
	}
	else if(pid == 0){
		/* 자식: 출력을 new fd로 리다이렉트 후 명령 실행 */
		dup2(new, old);
		execvp(args[0], args);
		fprintf(stderr, "exec error\n");
		exit(1);
	}
	else
		waitpid(pid, &status, 0);  /* 부모: 자식 완료 대기 */
}

/*
 * get_file_type() - 파일 확장자로 유형 판별
 *
 * .txt -> TEXTFILE (빈칸 채우기 문제)
 * .c   -> CFILE    (프로그램 문제)
 * 기타 -> -1       (지원하지 않는 유형)
 */
int get_file_type(char *filename)
{
	char *extension = strrchr(filename, '.');

	if(!strcmp(extension, ".txt"))
		return TEXTFILE;
	else if (!strcmp(extension, ".c"))
		return CFILE;
	else
		return -1;
}

/*
 * rmdirs() - 디렉토리 재귀 삭제
 *
 * 지정된 경로의 모든 파일과 하위 디렉토리를 재귀적으로 삭제한 뒤
 * 빈 디렉토리를 rmdir()로 제거한다. -e 옵션의 에러 디렉토리 초기화에 사용.
 */
void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[50];

	if((dp = opendir(path)) == NULL)
		return;

	while((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		if(lstat(tmp, &statbuf) == -1)
			continue;

		if(S_ISDIR(statbuf.st_mode))
			rmdirs(tmp);       /* 하위 디렉토리 재귀 삭제 */
		else
			unlink(tmp);       /* 파일 삭제 */
	}

	closedir(dp);
	rmdir(path);  /* 빈 디렉토리 삭제 */
}

/*
 * to_lower_case() - 대문자를 소문자로 변환
 *
 * ASCII 코드에서 대문자(A-Z)에 32를 더하면 소문자(a-z)가 된다.
 * compare_resultfile()에서 대소문자 무시 비교에 사용.
 */
void to_lower_case(char *c)
{
	if(*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

/*
 * print_usage() - 프로그램 사용법 출력
 *
 * -h 옵션 또는 잘못된 사용 시 호출된다.
 */
void print_usage()
{
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");
	printf(" -h                print usage\n");
	printf(" -p                print student's score and total average\n");
	printf(" -c <IDS>          print ID's score\n");
}
