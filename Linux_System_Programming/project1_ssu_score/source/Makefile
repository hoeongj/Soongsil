# Makefile - ssu_score 채점 시스템 빌드 스크립트
#
# 역할: main.c, ssu_score.c, blank.c를 컴파일하여
#       ssu_score 실행 파일을 생성한다.

# 최종 실행 파일 링킹
# [개선] -lpthread 추가: 학생 채점을 pthread 기반 병렬로 수행하기 위해 필요.
#        원본은 -lpthread 없이 순차 처리했으나, 개선본에서 pthread_create/join을
#        사용하므로 POSIX 스레드 라이브러리를 반드시 링크해야 한다.
ssu_score : main.o ssu_score.o blank.o
	gcc main.o ssu_score.o blank.o -o ssu_score -lpthread

# main.c 컴파일
# [개선] -O2 추가: GCC 최적화 레벨 2 적용.
#        인라인 확장, 루프 최적화 등으로 실행 속도를 향상시킨다.
main.o : main.c ssu_score.h
	gcc -O2 -c main.c

# ssu_score.c 컴파일 (가장 많은 개선이 적용된 핵심 모듈)
# [개선] -O2 추가: 최적화 레벨 2로 컴파일. blank.h 의존성도 명시.
ssu_score.o : ssu_score.c ssu_score.h blank.h
	gcc -O2 -c ssu_score.c

# blank.c 컴파일 (빈칸 채우기 채점 엔진)
# [개선] -O2 추가: 최적화 레벨 2로 컴파일.
blank.o : blank.c blank.h
	gcc -O2 -c blank.c

# clean 타겟: 빌드 산출물 제거
# [개선] rm -f 사용: 원본은 rm만 사용하여 파일이 없으면 에러가 발생했으나,
#        -f(force) 플래그 추가로 파일 부재 시에도 에러 없이 정상 종료한다.
#        이는 CI/CD 환경이나 반복 빌드 시 안정성을 보장한다.
clean :
	rm -f *.o
	rm -f ssu_score
