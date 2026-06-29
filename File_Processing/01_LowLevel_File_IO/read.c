#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

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

    if (byte == 0){
        close(fd);
        return 0;
    }

    int abs_byte = (byte < 0) ? -byte : byte;
    char *buf = (char *)malloc(abs_byte);
    ssize_t nread = 0;

    if(byte > 0){
        lseek(fd, offset + 1, SEEK_SET);
        nread = read(fd, buf, abs_byte); 
    }
    else{
        int start = offset + byte; 
        if(start < 0) start = 0;
        
        lseek(fd, start, SEEK_SET);
        
        int limited_byte = offset - start; 
        if (limited_byte < 0) limited_byte = 0;
        
        nread = read(fd, buf, limited_byte);
    }

    if(nread > 0){
        write(STDOUT_FILENO, buf, nread);
    }

    free(buf);
    close(fd);
    return 0;
}
