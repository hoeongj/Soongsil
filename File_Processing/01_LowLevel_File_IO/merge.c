#include <stdio.h> 
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if(argc != 4){
        fprintf(stderr, "Usage: %s <dest> <src1> <src2>\n", argv[0]);
        exit(1);
    }
    int fd1, fd2, fd3;
    char buf[1024];
    ssize_t nread;

    if((fd1 = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0){
        perror("open fd1");
        exit(1);
    }

    if((fd2 = open(argv[2], O_RDONLY)) < 0){
        perror("open fd2");
        exit(1);
    }

    if((fd3 = open(argv[3], O_RDONLY)) < 0){
        perror("open fd3");
        exit(1);
    }

   
    while((nread = read(fd2, buf, sizeof(buf))) > 0){
        write(fd1, buf, nread);
    }
    close(fd2);

    while((nread = read(fd3, buf, sizeof(buf))) > 0){
        write(fd1, buf, nread);
    }
    close(fd3);
    close(fd1);
    return 0;
}
