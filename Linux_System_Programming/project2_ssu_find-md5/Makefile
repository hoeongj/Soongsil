# ssu_clean 프로젝트 빌드 규칙.
# 메인 프롬프트, fmd5 명령어, help 명령어를 각각 실행 파일로 만든다.

CC      = gcc
CFLAGS  = -Wall -O2
CRYPTO_LIB ?= $(shell if [ -e /usr/lib/x86_64-linux-gnu/libcrypto.so ] || [ -e /usr/lib64/libcrypto.so ]; then printf '%s' '-lcrypto'; else printf '%s' '-l:libcrypto.so.3'; fi)
LDFLAGS = $(CRYPTO_LIB)

OBJS    = ssu_clean.o ssu_find-md5.o ssu_help.o
TARGETS = ssu_clean ssu_find-md5 ssu_help

all: $(TARGETS)

ssu_clean: ssu_clean.o
	$(CC) -o $@ ssu_clean.o

ssu_find-md5: ssu_find-md5.o
	$(CC) -o $@ ssu_find-md5.o $(LDFLAGS)

ssu_help: ssu_help.o
	$(CC) -o $@ ssu_help.o

ssu_clean.o: ssu_clean.c common.h
	$(CC) $(CFLAGS) -c ssu_clean.c

ssu_find-md5.o: ssu_find-md5.c common.h
	$(CC) $(CFLAGS) -c ssu_find-md5.c

ssu_help.o: ssu_help.c common.h
	$(CC) $(CFLAGS) -c ssu_help.c

clean:
	rm -f $(OBJS) $(TARGETS)
