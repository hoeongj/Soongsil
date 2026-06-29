/*
 * ssu_clean 메인 프롬프트.
 * 사용자 입력을 읽고 fmd5/help는 자식 프로세스로 실행하며, exit는 직접 종료한다.
 */


#include <sys/wait.h>
#include "common.h"

/* 제출 디렉토리에서 함께 빌드되는 실행 파일을 상대 경로로 호출한다. */
#define FMD5_PATH "./ssu_find-md5"
#define HELP_PATH "./ssu_help"

/* 내장 명령어 실행 파일을 fork 후 execv로 교체하고 부모는 종료를 기다린다. */
static void spawn_command(const char *path, char *argv[])
{
    /* 부모 프롬프트 프로세스에서 자식 프로세스를 하나 만든다. */
    pid_t pid = fork();
    if (pid < 0) {
        /* fork 자체가 실패하면 명령을 실행할 수 없으므로 에러만 출력하고 프롬프트로 돌아간다. */
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* 자식은 현재 프로세스 이미지를 명령어 실행 파일로 교체한다. */
        execv(path, argv);
        /* execv가 실패한 경우에만 에러를 출력하고 자식을 종료한다. */
        fprintf(stderr, "execv failed for %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* 부모는 다음 프롬프트를 출력하기 전에 자식이 끝날 때까지 기다린다. */
    int status;
    /* status 값은 따로 사용하지 않지만, 좀비 프로세스를 남기지 않기 위해 회수한다. */
    waitpid(pid, &status, 0);
}

/* 공백과 탭으로 입력을 나누어 execv에 넘길 argv 형태로 만든다. */
static int tokenize(char *input, char *argv[], int max_tokens)
{
    int argc = 0;
    /* 첫 토큰을 찾고, 이후 strtok(NULL, ...)로 같은 문자열을 계속 분리한다. */
    char *tok = strtok(input, " \t");
    while (tok != NULL && argc < max_tokens - 1) {
        /* argv 마지막 칸은 NULL 종료용으로 남겨 두고 토큰만 저장한다. */
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    /* execv가 요구하는 argv 형식처럼 NULL로 배열 끝을 표시한다. */
    argv[argc] = NULL;
    return argc;
}

int main(void)
{
    char input[INPUT_BUF_LEN];
    char *argv[MAX_TOKENS];

    /* 파이프 테스트에서 부모가 자식이 읽을 입력까지 미리 가져가지 않도록 한다. */
    setvbuf(stdin, NULL, _IONBF, 0);

    while (1) {
        /* 명세 형식의 프롬프트를 매 입력마다 출력한다. */
        printf("%s%s", STUDENT_ID, PROMPT_TAIL);
        fflush(stdout);

        /* EOF가 들어오면 줄을 정리하고 안전하게 종료한다. */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        /* Windows/Unix 개행을 모두 제거해 자동 테스트 입력도 동일하게 처리한다. */
        input[strcspn(input, "\r\n")] = '\0';

        /* 엔터만 입력한 경우 아무 명령도 실행하지 않고 프롬프트를 다시 출력한다. */
        if (input[0] == '\0') continue;

        /* 첫 토큰으로 내장 명령어를 구분한다. */
        int argc = tokenize(input, argv, MAX_TOKENS);
        if (argc == 0) continue;

        /* 첫 단어만 명령어로 보고 뒤의 토큰들은 fmd5 자식 프로세스에 그대로 넘긴다. */

        /* exit는 별도 프로세스를 만들지 않고 ssu_clean을 종료한다. */
        if (strcmp(argv[0], "exit") == 0) {
            printf("Prompt End\n");
            break;
        }

        /* fmd5는 탐색/삭제 로직을 가진 별도 실행 파일로 위임한다. */
        if (strcmp(argv[0], "fmd5") == 0) {
            spawn_command(FMD5_PATH, argv);
            continue;
        }

        /* help 역시 명세 요구에 맞춰 자식 프로세스로 실행한다. */
        if (strcmp(argv[0], "help") == 0) {
            spawn_command(HELP_PATH, argv);
            continue;
        }

        /* 알 수 없는 명령은 help를 실행한 것과 같은 결과를 보여준다. */
        char *help_argv[] = {"help", NULL};
        spawn_command(HELP_PATH, help_argv);
    }

    return 0;
}
