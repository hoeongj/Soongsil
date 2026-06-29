#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){
    if(argc != 4){
        fprintf(stderr, "Usage: %s <offset> <data> <filename>\n", argv[0]);
       exit(1); 
    }
    int offset = atoi(argv[1]);
    char *data = argv[2];
    char *filename = argv[3];
    int fd;
    if((fd = open(filename, O_WRONLY)) < 0){
        perror("open");
        exit(1);
    }

    lseek(fd, offset + 1, SEEK_SET);
    if(write(fd, data, strlen(data)) < 0){
        perror("write");
        exit(1);
    }

    close(fd);
    return 0;
}

