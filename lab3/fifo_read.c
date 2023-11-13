#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define PATH "./myfifo"
#define SIZE 128

int main()
{
    umask(0);
    if (mkfifo(PATH, 0666 | S_IFIFO) == -1) {
        perror ("mkefifo error");
        exit(0);
    }
    
    int fd = open (PATH,O_RDONLY);
    if (fd < 0) {
        printf("open fd is error\n");
        return 0;
    }

    char Buf[SIZE];
    while (1) {
        ssize_t s = read(fd, Buf, sizeof(Buf));
        if (s < 0) {
            perror("read error");
            exit(1);
        } else if (s == 0) {
            printf("client quit! i shoud quit!\n");
            break;
        } else {
            Buf[s] = '\0';
            printf("client# %s \n",Buf);
            fflush(stdout);
        }
    }

    close(fd);
    unlink(PATH);
    
    return 0;
}