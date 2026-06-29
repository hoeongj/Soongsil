#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]){
    if(argc != 4) exit(1);

    int offset = atoi(argv[1]);
    int byte = atoi(argv[2]);
    char *filename = argv[3];
    int fd;
    if((fd = open(filename, O_RDONLY)) < 0){
        perror("open");
        exit(1);
    }

    for(int i = 0 ; i < byte ; i++){
        char a;
        if(offset - i - 1 < 0) break;
        lseek(fd, offset - i - 1, SEEK_SET);
        if(read(fd, &a, 1) > 0){
            printf("%c",a);
        }
    }

    close(fd);
}
