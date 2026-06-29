#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    int fd1, fd2;
    char buf[5]; // for 5byte
    ssize_t nread;

    if(argc != 3){
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        exit(1);
    }

    if((fd1 = open(argv[1], O_RDONLY)) < 0){
        perror("open source");
        exit(1);
    }

    if((fd2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0){
        perror("open destination");
        exit(1);
    }

    while((nread = read(fd1, buf, 5)) > 0){
        write(fd2, buf, nread);
    }

    close(fd1);
    close(fd2);

    return 0;
}
