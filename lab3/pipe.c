#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

#define R_END 0
#define W_END 1

int main(int argc, char *argv[])
{
    int pid;
    int fd[2];
    char outpipe[100], inpipe[100];

    pipe(fd); // 创建管道

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else

    if (pid == 0) {
        close(fd[R_END]);

        lockf(fd[W_END], F_LOCK, 0); // 锁定管道
        
        sprintf(outpipe, "child process is sending message!");
        
        write(fd[W_END], outpipe, 50); // 向管道写入数据
        
        sleep(2);

        lockf(fd[W_END], F_ULOCK, 0); // 解锁管道
        
        exit(0);
    } else {
        wait(0); // 等待子进程结束

        read(fd[R_END], inpipe, 50); // 从管道读取数据

        printf("Parent: %s\n", inpipe);

        close(fd[R_END]);

        exit(0);
    }
}
