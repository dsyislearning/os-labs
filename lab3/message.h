#ifndef MESSAGE_H
#define MESSAGE_H

#define MSGKEY 1234

#define MAX_METEXT_SIZE 512

struct msg_st {
    long mtype;
    char mtext[MAX_METEXT_SIZE];
};

#endif // MESSAGE_H
