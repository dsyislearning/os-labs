# lab3 进程通信

## 实验环境

- OS: Arch Linux
- Kernel Version：6.5.9-arch2-1 (64-bit)

Linux 进程间通信（Inter-Process Communication）有 POSIX 标准类型和 System V（读作 System Five）标准类型两种。查阅资料有以下相关信息：

> 两者实现的进程间通信都有相同的基本工具——信号量、共享内存和消息队列。它们为这些工具提供的界面略有不同，但基本概念是相同的。一个值得注意的区别是，POSIX 为消息队列提供了一些 System V 所没有的通知功能（`mq_notify()`）。
>
> System V 标准的 IPC 已经存在了更长的时间，这有几个实际意义：
>
> - POSIX IPC 的实现使用范围并不非常广泛
> - POSIX IPC 是在System V IPC 使用一段时间之后设计的。因此，POSIX API 的设计者能够从 System V API 的优点和缺点中学习。因此，POSIX API 更简单、更易于使用
>
> 至于为什么有两个标准，POSIX 创建了他们的标准，因为他们认为这是对 System V 标准的改进。但是，如果每个人都同意 POSIX IPC 更好，那么许多程序仍然使用 System V IPC，并且将它们全部移植到 POSIX IPC 需要数年时间。在实践中，这是不值得的，所以即使从明天开始所有新代码都使用 POSIX IPC，System V IPC也会持续很多年。

综上所述，本实验学习使用 System V 标准的进程间通信。

值得注意的是，System V 相关的系统调用往往在 `sys/` 头文件目录下，例如进程间通信相关的系统调用在头文件 `sys/ipc.h` 中；而 POSIX 系统调用往往是以单独的头文件表示一个模块，例如消息队列函数在头文件 `mqueue.h` 中。

## 3-1 基于消息队列的进程通信

发送者进程向消息队列中写入数据，接收者进程从队列中接收数据，实现相互通信。消息队列克服了信号 signal 承载信息量少，管道 pipe 只能承载无格式字节流以及缓冲区大小受限等缺点。

### System V 消息队列

#### 创建/获取消息队列 `msgget`

`msgget()` 是 System V 消息队列 API 中的一个函数，用于创建或访问一个消息队列。

下面是 `msgget()` 函数的基本原型：

```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int msgget(key_t key, int msgflg);
```

参数说明：

- `key`：用于标识消息队列的键值。可以使用 `ftok()` 函数生成键值，也可以使用一个已知的键值。

- `msgflg`：标志位，用于指定消息队列的操作方式。可以使用不同的标志位做或运算来指定创建新队列、访问已有队列等操作。常用的标志位有：
  - `IPC_CREAT`：如果消息队列不存在，则创建一个新的队列。
  - `IPC_EXCL`：与 `IPC_CREAT` 结合使用，确保只创建一个新队列。
  - `0666`：用于指定消息队列的权限。

返回值：

- 如果成功，返回一个非负整数，表示消息队列的标识符（即队列描述符）。
- 如果失败，返回 -1，并设置相应的错误码。

`msgget()` 函数的作用是创建一个新的消息队列或访问一个已存在的消息队列。它根据给定的键值来识别和定位消息队列。如果键值对应的消息队列不存在，并且指定了 `IPC_CREAT` 标志位，那么 `msgget()` 函数将创建一个新的消息队列。如果消息队列已经存在，那么 `msgget()` 函数将返回对应的队列描述符。

下面是一个样例代码，展示如何使用 `msgget()` 函数创建一个新的消息队列：

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSGKEY 1234

int main() {
    int msgid;

    // 创建消息队列
    msgid = msgget(MSGKEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    printf("消息队列创建成功，标识符为：%d\n", msgid);

    return 0;
}
```

在这个案例中，首先通过 `msgget()` 函数创建一个新的消息队列，使用常量 `MSGKEY` 作为键值。然后输出获取的消息队列标识符。

需要注意的是，不同的进程可以通过使用相同的键值来访问同一个消息队列，从而实现进程间的通信。

#### 消息发送 `msgsnd`

`msgsnd()` 是 System V 消息队列 API 中的一个函数，用于向消息队列发送消息。

下面是 `msgsnd()` 函数的基本原型：

```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
```

参数说明：

- `msqid`：消息队列的标识符（队列描述符），由 `msgget()` 返回。
- `msgp`：指向要发送的消息的指针，通常是一个自定义的结构体指针，表示消息的内容。这个结构体必须包含一个长整型的 `mtype` 成员，用于指定消息的类型。
- `msgsz`：消息的大小（字节数），不包括 `mtype` 成员。
- `msgflg`：标志位，用于控制当前消息队列满或队列消息到达系统范围的限制时将要发生的事情。可以使用不同的标志位来控制消息发送的阻塞和非阻塞模式，以及其他选项。

返回值：

- 如果成功，返回 0。
- 如果失败，返回 -1，并设置相应的错误码。

下面是一个简单的代码样例，展示如何使用 `msgsnd()` 函数将消息添加到消息队列中：

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSGKEY 1234

// 定义消息结构体
struct message {
    long mtype;
    char mtext[100];
};

int main() {
    int msgid;
    struct message msg;

    // 获取消息队列标识符
    msgid = msgget(MSGKEY, 0666);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    printf("消息队列标识符：%d\n", msgid);

    // 设置消息类型
    msg.mtype = 1;
    // 设置消息内容
    sprintf(msg.mtext, "Hello, Message Queue!");

    // 发送消息
    if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        return 1;
    }

    printf("消息发送成功！\n");

    return 0;
}
```

在这个代码样例中，首先使用 `msgget()` 函数获取消息队列的标识符，该标识符通过键值常量 `MSGKEY` 指定。然后，设置消息的类型和内容，并使用 `msgsnd()` 函数将消息添加到消息队列中。

消息结构体需要满足以下要求：

1. 结构体中必须包含一个长整型 （`long`） 的 `mtype` 成员：`mtype` 用于指定消息的类型。它可以是一个正整数，用于识别不同类型的消息。在发送和接收消息时，可以使用 `mtype` 来过滤特定类型的消息。
2. 结构体中可以包含其他成员来表示消息的内容：除了 `mtype` 外，结构体可以包含其他成员，用于表示消息的实际内容。这些成员的类型和数量可以根据具体的需求进行定义。
3. 结构体的大小必须包括消息内容的大小：在使用 `msgsnd()` 函数发送消息时，需要指定消息的大小。因此，消息结构体的大小必须包括消息内容的大小，但不包括 `mtype` 成员的大小。

例如上面例子中的结构体中有两个成员变量，一个一定是 `long` 类型的 `mtype` ，另一个是自定义的 `char` 数组 `mtext` 。

需要注意的是，在实际使用中，需要保证发送消息的进程和接收消息的进程使用相同的消息队列标识符，以便进行进程间的通信。

#### 消息接收 `msgrcv`

`msgrcv()` 是 System V 消息队列 API 中的一个函数，用于从消息队列接收消息。

下面是 `msgrcv()` 函数的基本原型：

```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
```

参数说明：

- `msqid`：消息队列的标识符（队列描述符），由 `msgget()` 返回。

- `msgp`：指向接收消息的缓冲区的指针，通常是一个自定义的结构体指针，用于存储接收到的消息内容。

- `msgsz`：接收缓冲区的大小（字节数），必须大于等于消息的大小，不包括 `mtype` 成员。

- `msgtyp`：用于指定接收消息的类型。可以实现一种简单的接收优先级：
  - `0`：接收队列中的第一条消息。
  - `> 0`：接收队列中第一个与 `msgtyp` 相等的消息。
  - `< 0`：接收队列中第一个类型值小于等于 `msgtyp` 绝对值的消息。
  
- `msgflg`：标志位，用于指定接收消息的行为。可以使用不同的标志位来控制消息接收的阻塞和非阻塞模式，以及其他选项。

返回值：

- 如果成功，返回放到接收缓存区中的字节数，消息被复制到由 `msg_ptr` 指向的用户分配的缓存区中，然后删除消息队列中的对应消息。
- 如果失败，返回 -1，并设置相应的错误码。

下面是一个简单的代码样例，展示如何使用 `msgrcv()` 函数从消息队列接收消息：

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSGKEY 1234

// 定义消息结构体
struct message {
    long mtype;
    char mtext[100];
};

int main() {
    int msgid;
    struct message rcv_msg;

    // 获取消息队列标识符
    msgid = msgget(MSGKEY, 0666);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    printf("消息队列标识符：%d\n", msgid);

    // 接收消息
    ssize_t msg_size = msgrcv(msgid, &rcv_msg, sizeof(struct message) - sizeof(long), 0, 0);
    if (msg_size == -1) {
        perror("msgrcv");
        return 1;
    }

    printf("接收到的消息：%s\n", rcv_msg.mtext);

    return 0;
}
```

在这个代码样例中，首先使用 `msgget()` 函数获取消息队列的标识符，该标识符通过常量 `MSGKEY` 指定。然后，使用 `msgrcv()` 函数从消息队列中接收消息，并将接收到的消息存储在 `rcv_msg` 结构体中。

需要注意的是，在实际使用中，需要保证接收消息的进程和发送消息的进程使用相同的消息队列标识符和消息类型，以便进行进程间的通信。

#### 控制消息队列 `msgctl`

`msgctl()` 是 System V 消息队列 API 中的一个函数，用于对消息队列进行控制操作，如创建、删除、获取和修改消息队列的属性。

下面是 `msgctl()` 函数的基本原型：

```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int msgctl(int msqid, int cmd, struct msqid_ds *buf);
```

参数说明：

- `msqid`：消息队列的标识符（队列描述符），由 `msgget()` 返回。

- `cmd`：控制命令，用于指定要执行的操作类型。常用的命令包括：
  - `IPC_STAT`：获取消息队列的属性信息，并将其存储在 `buf` 参数指向的结构体中。
  - `IPC_SET`：设置消息队列的属性，使用 `buf` 参数指向的结构体中的信息进行修改。
  - `IPC_RMID`：删除消息队列。
  
- `buf`：指向 `struct msqid_ds` 结构体的指针，用于存储消息队列的属性信息或设置属性时的新值。

返回值：

- 如果成功，返回 0。
- 如果失败，返回 -1，并设置相应的错误码。

下面是一个简单的代码样例，展示如何使用 `msgctl()` 函数对消息队列进行控制操作：

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSGKEY 1234

int main() {
    int msgid;

    // 创建或获取消息队列
    msgid = msgget(MSGKEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    printf("消息队列标识符：%d\n", msgid);

    // 获取消息队列的属性信息
    struct msqid_ds queue_info;
    if (msgctl(msgid, IPC_STAT, &queue_info) == -1) {
        perror("msgctl");
        return 1;
    }

    printf("消息队列的消息数：%ld\n", queue_info.msg_qnum);
    printf("消息队列的最大字节数：%ld\n", queue_info.msg_qbytes);

    // 删除消息队列
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        return 1;
    }

    printf("消息队列已删除！\n");

    return 0;
}
```

在这个代码样例中，首先使用 `msgget()` 函数创建或获取一个消息队列，并将其标识符存储在 `msgid` 变量中。然后，使用 `msgctl()` 函数获取消息队列的属性信息，并将其存储在 `queue_info` 结构体中。最后，使用 `msgctl()` 函数删除消息队列。

需要注意的是，在实际使用中，需要根据具体的需求选择适当的命令和参数来执行不同的控制操作。

### 使用消息队列进行进程间通信

下面使用 System V 消息队列接口编程实现发送方和接收方两个并发进程。

发送方和接收方使用 `msgget` 、`msgctl` 创建、管理消息队列。只有接收者在接收完最后一个消息之后，才删除消息。

#### 发送方

自定义消息结构体及相关宏定义源文件为 `lab3/message.h` ，以便发送方程序和接收方程序复用，内容如下：

```c
#ifndef MESSAGE_H
#define MESSAGE_H

#define MSGKEY 1234

#define MAX_METEXT_SIZE 512

struct msg_st {
    long mtype;
    char mtext[MAX_METEXT_SIZE];
};

#endif // MESSAGE_H
```

发送者使用 `msgsend` 向消息队列不断写入数据，并打印提示信息。源文件为 `lab3/sender.c` ，代码内容如下：

```c
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
```

#### 接收方

接收方使用 `msgrcv` 从消息队列中接收消息，并打印提示信息。源文件为 `lab3/receiver.c` ，代码内容如下：

```c
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
```

#### 测试结果

编译运行 sender 和 receiver ，将 receiver 在当前会话的后台运行，再运行 sender ，输入字符串可以看到发送方发送什么接收方就输出什么，实现了进程间的消息通信。

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [23:50:24] 
$ gcc -o sender sender.c -lm

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [23:50:30] 
$ gcc -o receiver receiver.c -lm

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [23:50:34] 
$ ./receiver &
[1] 67847

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [23:50:44] 
$ ./sender    
Enter some text: Linux
You wrote: Linux
Enter some text: Programming
You wrote: Programming
Enter some text: end
You wrote: end
[1]  + 67847 done       ./receiver
```

## 3-2 共享内存通信

Linux 内核支持多种共享内存方式,如 mmap()系统调用, POSIX 共享内存,以及 System
V 共享内存。本实验采用 System V 共享内存实现方法。
要求:编程实现写者进程 Writer 和读者进程 Reader,
(1)写者进程和读者进程使用 shmget(key_t key, int size, int shmflg)创建在内存中创建用于
两者间通信的共享内存,使用 shmat(int shmid, char*shmaddr, int flag)将共享内存映射到进程
地址空间中,以便访问共享内存内容;
(2)写者进程向共享内存写入多组数据,读者进程从共享内存 d 读出数据;
(3)进程间通信结束后,写者进程和读者进程使用 shmdt(char*shmaddr)解除共享内存映射,
使用 shmctl(int shmid, int cmd, struct shmid_ds*buf)删除共享内存。
原理:
进程间需要共享的数据存放于一个称为 IPC 共享内存的内存区域,需要访问该共享区
域的进程需要将该共享内存区域映射到本进程的地址空间中。系统 V 共享内存通过 shmget
获得或创建一个 IPC 共享内存区域,并返回相应的标识符。内核执行 shmget 创建一个共享
内存区,初始化该共享内存区对应的 struct shmid_kernel 结构,同时在特殊文件系统 shm 中
创建并打开一个同名文件,并在内存中建立起该文件对应的 dentry 和 inode 结构。新打开的
文件不属于任何一个特定进程,任何进程都可以访问该共享内存区。
所创建的共享内存区有一个重要的控制结构 struct shmid_kernel,该结构联系内存管理
和文件系统的桥梁,该结构中最重要的域是 shm_file,存储了被映射文件的地址。每个共享
内存区对象都对应特殊文件系统 shm 中的一个文件,当采取共享内存的方式将 shm 中的文
件映射到进程地址空间后,可直接以访问内存的方式访问该文件。

### System V 共享内存





### 使用共享内存进行进程间通信



## 3-3 管道通信

管道是进程间共享的、用于相互间通信的文件,Linux/Unix 提供了 2 种类型管道:
(1)用于父进程-子进程间通信的无名管道。无名管道是利用系统调用 pipe(filedes)创
建的无路径名的临时文件,该系统调用返回值是用于标识管道文件的文件描述符 filedes,只
有调用 pipe(filedes)的进程及其子进程才能识别该文件描述符,并利用管道文件进行通信。
通过无名管道进行通信要注意读写端需要通过 lock 锁对管道的访问进行互斥控制。
(2)用于任意两个进程间通信的命名管道 named pipe。命名管道通过 mknod(const char
*path, mode_t mod, dev_t dev)、mkfifo(const char *path, mode_t mode)创建,是一个具有路径
名、在文件系统中长期存在的特殊类型的 FIFO 文件,读写遵循先进先出(first in first out),
管道文件读是从文件开始处返回数据,对管道的写是将数据添加到管道末尾。命名管道的名
字存在于文件系统中,内容存放在内存中。访问命名管道前,需要使用 open()打开该文件。
管道是单向的、半双工的,数据只能向一个方向流动,双向通信时需要建立两个管道。
一个命名管道的所有实例共享同一个路径名,但是每一个实例均拥有独立的缓存与句柄。只
要可以访问正确的与之关联的路径,进程间就能够彼此相互通信。
要求:编程实现进程间的(无名)管道、命名管道通信,方式如下。
(1)管道通信:
父进程使用 fork()系统调用创建子进程;
子进程作为写入端,首先将管道利用 lockf()加锁,然后向管道中写入数据,并解锁管道;
父进程作为读出端,从管道中读出子进程写入的数据。
(2)命名管道通信:
读者进程使用 mknod 或 mkfifo 创建命名管道;
读者进程、写者进程使用 open()打开创建的管道文件;
读者进程、写者进程分别使用 read()、write()读写命名管道文件中的内容;
通信完毕,读者进程、写者进程使用 close 关闭命名管道文件。





## 3-4 信号 signal 通信

使用信号相关的系统调用,参照后续样例程序展示的系统调用使用方法,设计实现基于
信号 signal 的进程通信。例如:
父进程使用系统调用 fork()函数创建两个子进程,再用系统调用 signal()函数通知父进程
捕捉键盘上传来的中断信号(即按 Del 键),当父进程接收到这两个软中断的其中某一个后,
父进程用系统调用 kill()向两个子进程分别发送整数值为 16 和 17 软中断信号,子进程获得
对应软中断信号后,分别输出下列信息后终止。