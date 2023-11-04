#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "message.h"

int main(int argc, char *argv[])
{
    struct msg_st message;
    char buffer[BUFSIZ];
    int msgid = -1;

    // 获取消息队列
    msgid = msgget((key_t)MSGKEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // 向消息队列中写入消息，直到写入 end
    while (true) {
        printf("Enter some text: ");
        fgets(buffer, BUFSIZ, stdin);
        message.mtype = 1;
        strcpy(message.mtext, buffer);

        // 向队列里发送数据
        if (msgsnd(msgid, (void *)&message, MAX_METEXT_SIZE, 0) == -1) {
            fprintf(stderr, "msgsnd failed\n");
            exit(EXIT_FAILURE);
        }

        // 输入 end 结束输入
        if (strncmp(buffer, "end", 3) == 0) {
            break;
        }

        sleep(1);
    }

    exit(EXIT_SUCCESS);
}
