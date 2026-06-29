/*
 * main.c - 프로그램 진입점
 *
 * 역할: ssu_score 채점 시스템의 메인 엔트리 포인트.
 *       프로그램 시작/종료 시각을 측정하여 총 실행 시간을 출력한다.
 *       실제 채점 로직은 ssu_score() 함수에 위임한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_score.h"

/*
 * SECOND_TO_MICRO: 1초를 마이크로초(microsecond)로 변환하는 상수.
 * timeval 구조체에서 tv_usec 빌림(borrow) 계산 시 사용한다.
 * 1초 = 1,000,000 마이크로초
 */
#define SECOND_TO_MICRO 1000000

/* ssu_runtime 함수 프로토타입: 시작/종료 시각을 받아 경과 시간을 계산·출력 */
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

/*
 * main() - 프로그램 진입점
 *
 * 동작 흐름:
 *   1) gettimeofday()로 시작 시각 기록
 *   2) ssu_score()를 호출하여 전체 채점 수행
 *   3) gettimeofday()로 종료 시각 기록
 *   4) ssu_runtime()으로 경과 시간 계산 및 출력
 */
int main(int argc, char *argv[])
{
	struct timeval begin_t, end_t;
	gettimeofday(&begin_t, NULL);  /* 채점 시작 전 현재 시각 기록 */

	ssu_score(argc, argv);  /* 메인 채점 로직 실행 */

	gettimeofday(&end_t, NULL);  /* 채점 완료 후 현재 시각 기록 */
	ssu_runtime(&begin_t, &end_t);  /* 경과 시간 계산 및 출력 */

	exit(0);
}

/*
 * ssu_runtime() - 경과 시간 계산 및 출력 함수
 *
 * timeval 구조체의 뺄셈을 수행한다.
 * tv_usec(마이크로초) 필드에서 빌림(borrow)이 필요한 경우
 * tv_sec에서 1초를 빌려 tv_usec에 1,000,000을 더한 뒤 뺄셈한다.
 *
 * 매개변수:
 *   begin_t - 시작 시각
 *   end_t   - 종료 시각 (계산 후 결과가 이 구조체에 저장됨)
 */
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	/* 초 단위 차이 계산 */
	end_t->tv_sec -= begin_t->tv_sec;

	/* 마이크로초 빌림 처리: end의 usec이 begin보다 작으면 1초를 빌린다 */
	if(end_t->tv_usec < begin_t->tv_usec){
		end_t->tv_sec--;                      /* 초에서 1 빌림 */
		end_t->tv_usec += SECOND_TO_MICRO;    /* 마이크로초에 1,000,000 추가 */
	}

	/* 마이크로초 차이 계산 */
	end_t->tv_usec -= begin_t->tv_usec;

	/* "초:마이크로초" 형식으로 실행 시간 출력. %06ld로 6자리 0-패딩 */
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}
