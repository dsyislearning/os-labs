#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "message.h"

int main(int argc, char *argv[])
{
    int msgid = -1;
    struct msg_st message;
    long mtype = 0;

    // 获取消息队列
    msgid = msgget((key_t)MSGKEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // 从队列中获取消息，直到遇到 end 消息为止
    while (true) {
        if (msgrcv(msgid, (void *)&message, MAX_METEXT_SIZE, mtype, 0) == -1) {
            fprintf(stderr, "msgrcv failed with errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }

        printf("You wrote: %s", message.mtext);

        if (strncmp(message.mtext, "end", 3) == 0) {
            break;
        }
    }

    // 删除消息队列
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "msgctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
