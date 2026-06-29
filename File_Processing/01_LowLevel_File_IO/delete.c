#define _DEFAULT_SOURCE
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

    if(byte <= 0) return 0;

    if((fd = open(filename, O_RDWR)) < 0){
        perror("open");
        exit(1);
    }

    long file_size = lseek(fd, 0, SEEK_END);
    long remaining = file_size - (offset + 1);
    if (remaining < 0) remaining = 0;
    if (byte > remaining) byte = remaining;

    long last_size = file_size - (offset + 1 + byte);
    if (last_size < 0) last_size = 0;

    if (last_size > 0) {
        char *last = malloc(last_size);
        lseek(fd, offset + 1 + byte, SEEK_SET);
        read(fd, last, last_size);
        lseek(fd, offset + 1, SEEK_SET);
        write(fd, last, last_size);
        free(last);
    }

    ftruncate(fd, file_size - byte);

    close(fd);
    return 0;
}
