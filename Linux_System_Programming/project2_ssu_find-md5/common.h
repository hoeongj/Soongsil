/*
 * 공통 헤더.
 * 세 실행 파일이 함께 쓰는 표준 헤더, 학번, 경로 제한, 프롬프트 문자열을 모아 둔다.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

/* ssu_clean 프롬프트에 출력할 학번이다. */
#define STUDENT_ID "20221528"

/* Linux 경로/파일명 제한을 명세 기준으로 고정한다. */
#define SSU_PATH_MAX 4096
#define SSU_NAME_MAX 255

/* 한 줄 명령어와 토큰 배열의 최대 크기다. */
#define INPUT_BUF_LEN 8192
#define MAX_TOKENS    64

/* MD5 16바이트를 32자리 16진수 문자열로 저장한다. */
#define MD5_HEX_LEN 33

/* t 옵션에서 중복 파일을 옮길 휴지통 디렉토리 이름이다. */
#define TRASH_DIR_NAME ".ssu_trash"

/* 메인 프롬프트와 삭제 서브 프롬프트 문자열이다. */
#define PROMPT_TAIL "> "
#define SUB_PROMPT  ">> "

#endif
