#include <stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>

#define PATH "./myfifo"
#define SIZE 128

int main()
{
    int fd = open(PATH, O_WRONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    char buf[SIZE];
    while (1) {
        printf("please enter # ");
        fflush(stdout);

        ssize_t s = read(0, buf, sizeof(buf));
        if (s < 0) {
            perror("read");
            return -1;
        } else {
            buf[s - 1] = 0;
            write(fd, buf, sizeof(buf));
        }
    }
    
    close(fd);

    return 0;
}
