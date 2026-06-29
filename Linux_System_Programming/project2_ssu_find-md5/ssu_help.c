/*
 * help 명령어.
 * 구현한 fmd5 옵션과 삭제 서브 명령어 사용법을 한 번에 출력한다.
 */

#include "common.h"

int main(int argc, char *argv[])
{
    /* help는 인자를 해석하지 않으므로 경고 방지를 위해 명시적으로 버린다. */
    (void)argc;
    (void)argv;

    /* 모든 도움말 출력은 Usage 제목 아래에 명세 순서대로 이어 붙인다. */
    printf("Usage:\n");

    /* 파일 확장자/크기 조건을 쓰는 기본 탐색 명령 형식이다. */
    printf("  > fmd5 -s [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
    printf("  > fmd5 -sh [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
    printf("  > fmd5 -s [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY] -a [ATIME_FROM] [ATIME_TO]\n");
    printf("  > fmd5 -s [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY] -m [MTIME_FROM] [MTIME_TO]\n");
    printf("  > fmd5 -s [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY] -c [CTIME_FROM] [CTIME_TO]\n");
    printf("  > fmd5 -sa/-sm/-sc [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY] [FROM] [TO]\n");
    printf("  > fmd5 -sah/-smh/-sch [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY] [FROM] [TO]\n");

    /* 시간 조건만으로 탐색하는 단독 옵션 형식이다. */
    printf("  > fmd5 -a [ATIME_FROM] [ATIME_TO] [TARGET_DIRECTORY]\n");
    printf("  > fmd5 -m [MTIME_FROM] [MTIME_TO] [TARGET_DIRECTORY]\n");
    printf("  > fmd5 -c [CTIME_FROM] [CTIME_TO] [TARGET_DIRECTORY]\n");
    printf("  > fmd5 -ah/-mh/-ch [FROM] [TO] [TARGET_DIRECTORY]\n");

    /* 하드링크 제외 옵션의 의미를 안내한다. */
    printf("       (-h: exclude hard links, can combine with -s/-a/-m/-c)\n");

    /* 추가 점수 대상 옵션도 구현되어 있으므로 함께 출력한다. */
    printf("       (-d [DEPTH] : limit BFS depth)\n");
    printf("       (-n [COUNT] : print only top COUNT largest sets)\n");
    printf("       (-e [DIR1] [[DIR2] [DIR3]] : exclude up to 3 directories from search)\n");
    printf("       (-p [MODE]  : only files with permission MODE)\n");
    printf("       (-r [size_desc|mtime_desc|path_asc] : sort duplicate sets)\n");
    printf("       (-o [FILE]  : save duplicate search result to FILE)\n");
    printf("       (-x [COUNT] : print sets with at least COUNT files)\n");

    /* 중복 세트가 발견된 뒤 사용할 삭제 명령어 형식이다. */
    printf("    >> [SET_INDEX] [OPTION ... ]\n");
    printf("        [OPTION ... ]\n");
    printf("        d [LIST_IDX] : delete [LIST_IDX] file\n");
    printf("        i            : ask for confirmation before delete\n");
    printf("        f [-u RULE]  : force delete except selected file\n");
    printf("        t [-u RULE]  : force move to Trash except selected file\n");
    printf("        -u RULE      : newest|oldest|pathshort|pathlong\n");
    printf("        exit         : back to prompt\n");

    /* 메인 프롬프트에서 사용할 수 있는 나머지 명령어다. */
    printf("  > help\n");
    printf("  > exit\n");

    /* help 출력만 수행하는 프로그램이므로 정상 종료한다. */
    return 0;
}
