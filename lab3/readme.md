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

Linux 内核支持多种共享内存方式，如 mmap() 系统调用，POSIX 共享内存，以及 System V 共享内存。本实验采用 System V 共享内存实现方法。

要求：编程实现写者进程 Writer 和读者进程 Reader,

(1) 写者进程和读者进程使用 `shmget(key_t key, int size, int shmflg)` 创建在内存中创建用于两者间通信的共享内存，使用 `shmat(int shmid, char*shmaddr, int flag)` 将共享内存映射到进程地址空间中，以便访问共享内存内容；

(2) 写者进程向共享内存写入多组数据，读者进程从共享内存读出数据；

(3) 进程间通信结束后，写者进程和读者进程使用 `shmdt(char*shmaddr)` 解除共享内存映射，使用 `shmctl(int shmid, int cmd, struct shmid_ds*buf)` 删除共享内存。

原理：

进程间需要共享的数据存放于一个称为 IPC 共享内存的内存区域，需要访问该共享区域的进程需要将该共享内存区域映射到本进程的地址空间中。System V 共享内存通过 `shmget` 获得或创建一个 IPC 共享内存区域，并返回相应的标识符。内核执行 `shmget` 创建一个共享内存区，初始化该共享内存区对应的 `struct shmid_kernel` 结构，同时在特殊文件系统 `shm` 中创建并打开一个同名文件，并在内存中建立起该文件对应的 `dentry` 和 `inode` 结构。新打开的文件不属于任何一个特定进程，任何进程都可以访问该共享内存区。

```c
strnuct shmid_kernel /"private to the kernel"/
{
	struct kern_ipc_perm shm_perm;
	struct file* shm_file;
	int id;
	unsigned long shm_nattch;
	unsigned long shm_segsz;
	time_t shm_atim;
	time_t shm_dtim;
	time_t shm_ctim;
	pid_t shm_cprd;
	pid_t shm_lprid;
};
```

所创建的共享内存区有一个重要的控制结构 `struct shmid_kernel`，该结构联系内存管理和文件系统的桥梁，该结构中最重要的域是 `shm_file` ，存储了被映射文件的地址。每个共享内存区对象都对应特殊文件系统 `shm` 中的一个文件，当采取共享内存的方式将 `shm` 中的文件映射到进程地址空间后，可直接以访问内存的方式访问该文件。

### System V 共享内存

#### 创建共享内存 `shmget`

`shmget()`函数用于创建或获取一个共享内存标识符（identifier）。共享内存标识符是一个整数值，用于唯一标识一块共享内存区域。

`shmget()`函数的原型如下：

```c
#include <sys/ipc.h>
#include <sys/shm.h>

int shmget(key_t key, size_t size, int shmflg);
```

参数说明：

- `key`：用于标识共享内存的键值，可以使用`ftok()`函数生成。不同的进程可以通过相同的键值来获取同一个共享内存区域。
- `size`：指定共享内存的大小，以字节为单位。
- `shmflg`：用于指定共享内存的权限和创建标志。可以使用 IPC_CREAT 标志来创建共享内存，使用 IPC_EXCL 标志来确保创建新的共享内存而不是获取已存在的共享内存。

`shmget()`函数的返回值是共享内存标识符（非负整数），失败返回 -1 。可以通过该标识符在其他函数中对共享内存进行操作，比如附加到进程地址空间、读写数据等。

需要注意的是，System V共享内存不提供任何同步机制。在多个进程同时访问共享内存时，需要使用其他的同步手段（如信号量、互斥锁等）来保证数据的一致性和并发访问的正确性。

#### 映射共享内存 `shmat`

`shmat()`函数用于将共享内存附加到进程的地址空间，使得进程可以访问和操作共享内存中的数据。它的原型如下：

```c
#include <sys/types.h>
#include <sys/shm.h>

void *shmat(int shmid, const void *shmaddr, int shmflg);
```

参数说明：

- `shmid`：共享内存标识符，通过`shmget()`函数获取。
- `shmaddr`：指定共享内存的附加地址。通常将其设置为`NULL`，由系统自动选择一个合适的地址。
- `shmflg`：附加标志，可以设置为`SHM_RDONLY`以只读模式附加共享内存。

`shmat()`函数返回一个指向共享内存起始地址的指针，或者返回`-1`表示失败。

使用`shmat()`函数将共享内存附加到进程的地址空间后，进程可以像访问普通内存一样直接操作共享内存中的数据。这意味着进程可以读取和写入共享内存中的数据，而无需进行复制或使用其他复杂的通信机制。

需要注意的是，一旦共享内存附加到进程的地址空间，它将一直保持附加状态，直到进程显式地使用`shmdt()`函数将其分离。当不再需要访问共享内存时，应该调用`shmdt()`函数将其从进程的地址空间中分离，以释放系统资源。

下面是一个示例代码，展示了如何使用`shmat()`函数将共享内存附加到进程的地址空间：

```c
#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>

int main() {
    int shmid;
    char *shared_memory;
    
    // 获取共享内存标识符
    shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    
    // 将共享内存附加到进程的地址空间
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) {
        perror("shmat");
        return 1;
    }
    
    // 使用共享内存进行读写操作
    printf("Shared memory attached at address %p\n", shared_memory);
    sprintf(shared_memory, "Hello, shared memory!");
    
    // 分离共享内存
    if (shmdt(shared_memory) == -1) {
        perror("shmdt");
        return 1;
    }
    
    return 0;
}
```

在上面的示例中，首先使用`shmget()`函数获取共享内存标识符，然后使用`shmat()`函数将共享内存附加到进程的地址空间。接下来，我们可以像使用普通指针一样使用`shared_memory`指针来读写共享内存中的数据。最后，使用`shmdt()`函数将共享内存从进程的地址空间中分离。

#### 共享内存解除映射 `shmdt`

`shmdt()`函数用于将共享内存从进程的地址空间中分离，即将共享内存与进程之间的关联解除。它的原型如下：

```c
#include <sys/types.h>
#include <sys/shm.h>

int shmdt(const void *shmaddr);
```

参数说明：

- `shmaddr`：指向共享内存起始地址的指针，即通过`shmat()`函数返回的指针。

`shmdt()`函数返回 0 表示成功，返回 -1 表示失败。

使用`shmdt()`函数将共享内存分离后，进程将不再能够访问和操作共享内存中的数据。分离共享内存并不会删除共享内存区域，只是解除了共享内存与进程之间的关联。

下面是一个示例代码，展示了如何使用`shmdt()`函数将共享内存从进程的地址空间中分离：

```c
#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>

int main() {
    int shmid;
    char *shared_memory;
    
    // 获取共享内存标识符
    shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    
    // 将共享内存附加到进程的地址空间
    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) {
        perror("shmat");
        return 1;
    }
    
    // 使用共享内存进行读写操作
    printf("Shared memory attached at address %p\n", shared_memory);
    sprintf(shared_memory, "Hello, shared memory!");
    
    // 分离共享内存
    if (shmdt(shared_memory) == -1) {
        perror("shmdt");
        return 1;
    }
    
    // 在分离后，不能再访问共享内存
    printf("Shared memory detached from process\n");
    
    return 0;
}
```

在上面的示例中，我们首先使用`shmget()`函数获取共享内存标识符，然后使用`shmat()`函数将共享内存附加到进程的地址空间。接下来，我们可以使用`shared_memory`指针读写共享内存中的数据。最后，使用`shmdt()`函数将共享内存从进程的地址空间中分离。

需要注意的是，分离共享内存并不会自动删除共享内存区域。如果不再需要使用共享内存，可以使用`shmctl()`函数进行删除操作。

#### 控制释放 `shmctl`

`shmctl()`函数用于对共享内存进行控制操作，例如获取共享内存信息、更改权限、删除共享内存等。它的原型如下：

```c
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int shmctl(int shmid, int cmd, struct shmid_ds *buf);
```

参数说明：

- `shmid`：共享内存标识符，通过`shmget()`函数获取。

- `cmd`：控制命令，用于指定要执行的操作。常用的命令有：
  - `IPC_STAT`：获取共享内存的信息，并将其存储在`buf`指向的`struct shmid_ds`结构体中。
  - `IPC_SET`：更改共享内存的权限和其他属性，`buf`指向一个`struct shmid_ds`结构体，提供要设置的新值。
  - `IPC_RMID`：删除共享内存。
  
- `buf`：指向`struct shmid_ds`结构体的指针，用于存储共享内存的信息或提供新的属性值。

`shmctl()`函数返回 0 表示成功，返回 -1 表示失败。

### 使用共享内存进行进程间通信

(1) 开启一个终端，创建两个程序：shm_read.c 及 shm_write.c；

(2) 编译这两个程序，然后执行

#### 写端

shm_write.c

```c
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char name[8];
    int age;
} people;

int main(int argc, char *argv[])
{
    int shm_id;
    key_t key;
    
    char temp[8];
    people * p_map;
    
    char pathname[30];
    strcpy(pathname, "/tmp");
    
    key = ftok(pathname, 0x04);
    if (key == -1) {
        perror("ftok error");
        return -1;
    }
    
    printf("key=%d\n", key);

    // 创建共享内存
    shm_id = shmget(key, 4096, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        perror("shmget error");
        return -1;
    }

    printf("shm_id=%d\n", shm_id);

    // 将共享内存映射到当前进程的地址空间
    p_map = (people *)shmat(shm_id, NULL, 0);

    memset(temp, 0, sizeof(temp));
    strncpy(temp, "test", 4);
    temp[4] = '\0';

    // 向共享内存中写入数据
    for (int i = 0; i < 3; i++) {
        temp[4] += 1;
        strncpy((p_map + i)->name, temp, 5);
        (p_map + i)->age = 0 + i;
    }

    // 断开与共享内存附加点的地址
    if (shmdt(p_map) == -1) {
        perror("detach error");
        return -1;
    }

    return 0;
}
```

#### 读端

shm_read.c

```c
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char name[8];
    int age;
} people;

int main(int argc, char *argv[])
{
    int shm_id;
    key_t key;
    people * p_map;
    char pathname[30];
    strcpy(pathname, "/tmp");

    key = ftok(pathname, 0x04);
    if (key == -1) {
        perror("ftok error");
        return -1;
    }

    printf("key=%d\n", key);

    // 创建共享内存
    shm_id = shmget(key, 0, 0);
    if (shm_id == -1) {
        perror("shmget error");
        return -1;
    }

    printf("shm_id=%d\n", shm_id);

    // 将共享内存映射到当前进程的地址空间
    p_map = (people *)shmat(shm_id, NULL, 0);

    // 从共享内存中读取数据
    for (int i = 0; i < 3; i++) {
        printf("name=%s, age=%d\n", (p_map + i)->name, (p_map + i)->age);
    }

    // 断开与共享内存附加点的地址
    if (shmdt(p_map) == -1) {
        perror("detach error");
        return -1;
    }

    // 删除共享内存
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl error");
        return -1;
    }

    return 0;
}
```

#### 运行结果

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [8:28:50]
$ gcc -o shm_write shm_write.c

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [8:29:50] 
$ gcc -o shm_read shm_read.c  

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [8:29:53] 
$ ./shm_write                 
key=69664769
shm_id=65564

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [8:29:55] 
$ ./shm_read                  
key=69664769
shm_id=65564
name=test, age=0
name=test, age=1
name=test, age=2
```

## 3-3 管道通信

管道是进程间共享的、用于相互间通信的文件，Linux/Unix 提供了 2 种类型管道：

(1) 用于父进程-子进程间通信的无名管道。无名管道是利用系统调用 `pipe(filedes)` 创建的无路径名的临时文件，该系统调用返回值是用于标识管道文件的文件描述符 `filedes` ，只有调用 `pipe(filedes)` 的进程及其子进程才能识别该文件描述符，并利用管道文件进行通信。通过无名管道进行通信要注意读写端需要通过 lock 锁对管道的访问进行互斥控制。

(2) 用于任意两个进程间通信的命名管道 named pipe。命名管道通过 `mknod(const char *path, mode_t mod, dev_t dev)` 、 `mkfifo(const char *path, mode_t mode)` 创建，是一个具有路径名、在文件系统中长期存在的特殊类型的 FIFO 文件，读写遵循先进先出（first in first out），管道文件读是从文件开始处返回数据，对管道的写是将数据添加到管道末尾。命名管道的名字存在于文件系统中，内容存放在内存中。访问命名管道前，需要使用 `open()` 打开该文件。

管道是单向的、半双工的，数据只能向一个方向流动，双向通信时需要建立两个管道。一个命名管道的所有实例共享同一个路径名，但是每一个实例均拥有独立的缓存与句柄。只要可以访问正确的与之关联的路径，进程间就能够彼此相互通信。

要求：编程实现进程间的(无名)管道、命名管道通信，方式如下。

(1) 管道通信：

- 父进程使用 `fork()` 系统调用创建子进程；

- 子进程作为写入端,首先将管道利用 `lockf()` 加锁,然后向管道中写入数据，并解锁管道；

- 父进程作为读出端，从管道中读出子进程写入的数据。

(2) 命名管道通信：

- 读者进程使用 `mknod` 或 `mkfifo` 创建命名管道；
- 读者进程、写者进程使用 `open()` 打开创建的管道文件；
- 读者进程、写者进程分别使用 `read()`、`write()` 读写命名管道文件中的内容；
- 通信完毕,读者进程、写者进程使用 `close()` 关闭命名管道文件。

### 管道相关系统调用

#### 创建匿名管道 `pipe()`

`pipe()`是一个系统调用，用于创建一个管道，是一种用于进程间通信的机制。管道提供了一个单向的、字节流的通道，可以用于在相关的进程之间传递数据。

`pipe()`函数的原型如下：

```c
#include <unistd.h>

int pipe(int pipefd[2]);
```

参数`pipefd`是一个整型数组，它包含了两个文件描述符。`pipefd[0]`用于从管道读取数据，`pipefd[1]`用于向管道写入数据。

`pipe()`函数返回0表示成功，返回-1表示失败。

下面是一个简单的示例，演示了如何使用`pipe()`函数创建一个管道，并在父子进程之间进行通信：

```c
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char message[] = "Hello, pipe!";
    char buffer[100];

    // 创建管道
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    // 创建子进程
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // 子进程从管道中读取数据
        close(pipefd[1]);  // 关闭写入端

        // 从管道中读取数据
        read(pipefd[0], buffer, sizeof(buffer));

        printf("Child process received: %s\n", buffer);

        close(pipefd[0]);  // 关闭读取端
    } else {
        // 父进程向管道中写入数据
        close(pipefd[0]);  // 关闭读取端

        // 向管道中写入数据
        write(pipefd[1], message, strlen(message) + 1);

        close(pipefd[1]);  // 关闭写入端
    }

    return 0;
}
```

在上面的示例中，我们首先使用`pipe()`函数创建了一个管道。然后，使用`fork()`函数创建了一个子进程。在子进程中，我们关闭了管道的写入端，然后使用`read()`函数从管道中读取数据，并打印出来。在父进程中，我们关闭了管道的读取端，然后使用`write()`函数向管道中写入数据。

通过这种方式，父进程和子进程可以通过管道进行通信。父进程将消息写入管道，子进程从管道中读取消息。

需要注意的是，管道是一种单向的通信机制，因此数据只能在一个方向上传递。如果需要双向通信，可以使用两个管道，或者使用其他的进程间通信机制，如命名管道（FIFO）或套接字（Socket）等。

#### 读取数据 `read()`

`read()`是一个系统调用，用于从文件描述符中读取数据。它的原型如下：

```c
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count);
```

参数说明：

- `fd`：要读取的文件描述符，可以是文件、管道、套接字等。
- `buf`：用于存储读取数据的缓冲区的指针。
- `count`：要读取的字节数。

`read()`函数返回读取的字节数。如果返回值为0，表示已达到文件末尾（EOF）。如果返回值为-1，表示读取出现错误，可以通过`errno`变量获取错误码。

下面是一个简单的示例，演示了如何使用`read()`函数从文件中读取数据：

```c
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main() {
    int fd;
    char buffer[100];
    ssize_t bytesRead;

    // 打开文件
    fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 从文件中读取数据
    bytesRead = read(fd, buffer, sizeof(buffer));
    if (bytesRead == -1) {
        perror("read");
        return 1;
    }

    // 打印读取的数据
    printf("Read %zd bytes: %s\n", bytesRead, buffer);

    // 关闭文件
    close(fd);

    return 0;
}
```

在上面的示例中，我们首先使用`open()`函数打开了一个文件，指定了只读模式。然后，使用`read()`函数从文件中读取数据，并将其存储在`buffer`中。最后，打印读取的数据和字节数，并使用`close()`函数关闭文件。

需要注意的是，`read()`函数是一个阻塞调用，如果没有数据可读，它将阻塞等待，直到有数据可读或者发生错误。如果需要非阻塞读取，可以使用`fcntl()`函数设置文件描述符的属性为非阻塞模式，或者使用多路复用机制如`select()`或`poll()`等进行读取操作的监视。

#### 写入数据 `write()`

`write()`是一个系统调用，用于将数据写入文件描述符。它的原型如下：

```c
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t count);
```

参数说明：

- `fd`：要写入数据的文件描述符，可以是文件、管道、套接字等。
- `buf`：要写入的数据的缓冲区的指针。
- `count`：要写入的字节数。

`write()`函数返回实际写入的字节数。如果返回值为-1，表示写入出现错误，可以通过`errno`变量获取错误码。

下面是一个简单的示例，演示了如何使用`write()`函数将数据写入文件：

```c
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main() {
    int fd;
    char message[] = "Hello, write!";
    ssize_t bytesWritten;

    // 打开文件
    fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 将数据写入文件
    bytesWritten = write(fd, message, sizeof(message) - 1);
    if (bytesWritten == -1) {
        perror("write");
        return 1;
    }

    printf("Written %zd bytes\n", bytesWritten);

    // 关闭文件
    close(fd);

    return 0;
}
```

在上面的示例中，我们首先使用`open()`函数打开了一个文件，指定了写入模式。如果文件不存在，使用`O_CREAT`标志创建文件。然后，使用`write()`函数将`message`中的数据写入文件。注意，我们在`sizeof(message) - 1`中减去1，是为了排除字符串末尾的空字符。最后，打印实际写入的字节数，并使用`close()`函数关闭文件。

需要注意的是，`write()`函数是一个阻塞调用，如果无法立即写入所有数据，它将阻塞等待，直到数据完全写入或者发生错误。如果需要非阻塞写入，可以使用`fcntl()`函数设置文件描述符的属性为非阻塞模式，或者使用多路复用机制如`select()`或`poll()`等进行写入操作的监视。

进程间通过管道用 write 和 read 来传递数据，但 write 和 read 不能同时进行，在管道中只能有 4096 字节数据被缓冲。

#### 进程休眠 `sleep()`

`sleep()`是一个系统调用，用于使当前进程挂起（暂停执行）一定的时间。它的原型如下：

```c
#include <unistd.h>

unsigned int sleep(unsigned int seconds);
```

参数`seconds`表示要挂起的时间长度，以秒为单位。函数返回值为未休眠的剩余时间，如果返回0，则表示已经完全休眠。

`sleep()`函数会导致当前进程进入阻塞状态，直到指定的时间过去。在此期间，操作系统会将CPU资源分配给其他可运行的进程，以提高系统的资源利用率。当指定的时间过去后，进程会从睡眠状态恢复，并继续执行后续的代码。

下面是一个简单的示例，演示了如何使用`sleep()`函数暂停程序的执行：

```c
#include <unistd.h>
#include <stdio.h>

int main() {
    printf("Start\n");

    // 暂停执行 3 秒
    sleep(3);

    printf("End\n");

    return 0;
}
```

在上面的示例中，我们调用了`sleep(3)`，使程序暂停执行 3 秒。在开始和结束的打印语句之间的时间间隔将是 3 秒。

需要注意的是，`sleep()`函数的精度是以秒为单位，因此无法实现精确的毫秒级延迟。如果需要实现更细粒度的延迟，可以使用其他函数或工具，如`nanosleep()`函数、定时器、线程等。

#### 文件锁 `lockf()`

`lockf()`是一个系统调用，用于对文件进行加锁或解锁操作。它的原型如下：

```c
#include <unistd.h>

int lockf(int fd, int cmd, off_t len);
```

参数说明：

- `fd`：要进行加锁或解锁的文件描述符。

- `cmd` ：指定要进行的操作，可以是以下值之一：
  - `F_LOCK`：对文件进行加锁。
  - `F_ULOCK`：解除文件的锁定。
  - `F_TLOCK`：尝试对文件进行加锁，如果文件已被锁定则阻塞。
  - `F_TEST`：测试文件的锁定状态，如果文件已被锁定则返回-1，否则返回0。
  
- `len`：指定要加锁或解锁的字节数，通常设置为0表示对整个文件进行操作。

`lockf()`函数返回值为0表示操作成功，返回-1表示操作失败。

下面是一个简单的示例，演示了如何使用`lockf()`函数对文件进行加锁和解锁操作：

```c
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main() {
    int fd;
    char message[] = "Locked file!";
    ssize_t bytesWritten;

    // 打开文件
    fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 加锁
    if (lockf(fd, F_LOCK, 0) == -1) {
        perror("lockf");
        return 1;
    }

    // 将数据写入文件
    bytesWritten = write(fd, message, sizeof(message) - 1);
    if (bytesWritten == -1) {
        perror("write");
        return 1;
    }

    printf("Written %zd bytes\n", bytesWritten);

    // 解锁
    if (lockf(fd, F_ULOCK, 0) == -1) {
        perror("lockf");
        return 1;
    }

    // 关闭文件
    close(fd);

    return 0;
}
```

在上面的示例中，我们首先使用`open()`函数打开了一个文件，指定了写入模式。如果文件不存在，使用`O_CREAT`标志创建文件。然后，使用`lockf()`函数对文件进行加锁操作，通过传递`F_LOCK`作为`cmd`参数。接着，使用`write()`函数将数据写入文件。最后，通过再次调用`lockf()`函数并传递`F_ULOCK`作为`cmd`参数，对文件进行解锁操作。

需要注意的是，`lockf()`函数对文件的加锁和解锁是针对当前进程有效的。如果同一文件被多个进程打开，每个进程需要分别调用`lockf()`函数进行加锁和解锁。

另外，`lockf()`函数提供了简单的文件级别的锁定机制，但不适用于进程间的同步和互斥操作。对于需要进程间同步的场景，应该使用更高级的锁机制，如文件锁（`fcntl()`函数的`F_SETLK`和`F_SETLKW`命令）、命名信号量、互斥量等。

#### 创建设备文件 `mknod`

`mknod()`是一个系统调用，用于创建设备文件。它的原型如下：

```c
#include <sys/types.h>
#include <sys/stat.h>

int mknod(const char *pathname, mode_t mode, dev_t dev);
```

参数说明：

- `pathname`：要创建的设备文件的路径名。
- `mode`：设备文件的权限模式，指定文件的访问权限和其他属性。
- `dev`：指定设备文件的主设备号和次设备号。

`mknod()`函数返回值为0表示操作成功，返回-1表示操作失败。

设备文件是用于与设备进行通信的特殊文件，包括块设备和字符设备。块设备是以固定大小的块为单位进行读写的设备，如硬盘驱动器；字符设备则是以字符为单位进行读写的设备，如串口。

下面是一个简单的示例，演示了如何使用`mknod()`函数创建设备文件：

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int main() {
    mode_t mode = S_IFCHR | 0666; // 字符设备文件权限
    dev_t dev = makedev(1, 3); // 主设备号和次设备号

    if (mknod("/dev/mydevice", mode, dev) == -1) {
        perror("mknod");
        return 1;
    }

    printf("Device file created\n");

    return 0;
}
```

在上面的示例中，我们使用了`makedev()`函数创建了一个`dev_t`类型的设备号，主设备号为1，次设备号为3。然后，调用`mknod()`函数创建了一个名为"/dev/mydevice"的字符设备文件，指定了权限模式`mode`和设备号`dev`。

需要注意的是，创建设备文件通常需要特权权限，因此需要以root用户或具有相应权限的用户身份运行程序。此外，创建设备文件是一个高级操作，应该谨慎使用，并了解设备文件的相关知识和操作。在大多数情况下，应该使用设备文件的驱动程序或相关工具来自动创建和管理设备文件。

#### 创建命名管道 `mkfifo`

`mkfifo()`是一个系统调用，用于创建命名管道（Named Pipe）。命名管道是一种特殊类型的文件，用于进程间进行通信。它的原型如下：

```c
#include <sys/types.h>
#include <sys/stat.h>

int mkfifo(const char *pathname, mode_t mode);
```

参数说明：

- `pathname`：要创建的命名管道的路径名。
- `mode`：命名管道的权限模式，指定文件的访问权限和其他属性。

`mkfifo()`函数返回值为0表示操作成功，返回-1表示操作失败。

命名管道提供了进程间的单向或双向通信机制，其中一个进程可以将数据写入管道，另一个进程可以从管道中读取数据。与匿名管道（使用`pipe()`函数创建）不同，命名管道使用文件系统中的文件作为其表示，并且可以被多个进程同时访问。

下面是一个简单的示例，演示了如何使用`mkfifo()`函数创建命名管道：

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int main() {
    mode_t mode = 0666; // 管道文件权限

    if (mkfifo("/tmp/myfifo", mode) == -1) {
        perror("mkfifo");
        return 1;
    }

    printf("Named pipe created\n");

    return 0;
}
```

在上面的示例中，我们调用了`mkfifo()`函数来创建一个名为 "/tmp/myfifo" 的命名管道文件，并指定了权限模式`mode`。

需要注意的是，命名管道是一个阻塞的通信机制，当一个进程试图从空管道中读取数据或向已满的管道中写入数据时，它将被阻塞，直到有数据可读或管道有足够的空间可写入。因此，在使用命名管道进行进程间通信时，需要确保读取和写入操作在合适的时机进行，以避免进程阻塞。

此外，命名管道文件是持久的，即使没有进程打开它也会保留在文件系统中。当不再需要命名管道时，应该使用`unlink()`函数删除它。

### 匿名管道在父子进程间通信

#### 父子进程使用匿名管道

子进程锁定管道并向管道内写入数据，写完后解锁。

父进程从管道读取数据。

```c
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
```

#### 运行结果

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [8:29:57] 
$ gcc -o pipe pipe.c                        

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [9:10:25] 
$ ./pipe       
Parent: child process is sending message!
```

### 命名管道在独立进程间通信

#### 写进程

fifo_write.c

```c
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
```

#### 读进程

fifo_read.c

```c
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
```

#### 运行结果

先运行读端：

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [9:30:56] 
$ gcc -o fifo_read fifo_read.c  

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [9:31:10] 
$ ./fifo_read 
client# hello 
client# world 
client quit! i shoud quit!
```

再运行写端并写入数据

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [9:30:59] 
$ gcc -o fifo_write fifo_write.c

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [9:31:14] 
$ ./fifo_write 
please enter # hello
please enter # world
please enter # ^C
```

## 3-4 信号 signal 通信

### Linux signal 软中断机制及 API

软中断信号（signal，又简称为信号）用来通知进程发生了异步事件。进程之间可以互相通过系统调用 `kill` 发送软中断信号。内核也可以因为内部事件而给进程发送信号，通知进程发生了某个事件。注意，信号只是用来通知某进程发生了什么事件，并不给该进程传递任何数据。

收到信号的进程对各种信号有不同的处理方法。处理方法可以分为三类：

- 第一种是类似中断的处理程序，对于需要处理的信号，进程可以指定处理函数，由该函数来处理。
- 第二种方法是，忽略某个信号，对该信号不做任何处理，就像未发生过一样。
- 第三种方法是，对该信号的处理保留系统的默认值，这种缺省操作，对大部分的信号的缺省操作是使得进程终止。进程通过系统调用 signal 来指定进程对某个信号的处理行为。

在进程表的 PCB 表项中有一个软中断信号域，该域中每一位对应一个信号，当有信号发送给进程时，对应位置位。由此可以看出，进程对不同的信号可以同时保留，但对于同一个信号，进程并不知道在处理之前来过多少个。

### 信号的系统调用

系统调用 `signal` 是进程用来设定某个信号的处理方法，系统调用 `kill` 是用来发送信号给指定进程的。这两个调用可以形成信号的基本操作。后两个调用 `pause` 和 `alarm` 是通过信号实现的进程暂停和定时器，调用 `alarm` 是通过信号通知进程定时器到时。

#### 设定信号处理方法 `signal`

`signal()`用于设置信号处理函数。它用于在程序中捕获和处理各种信号。信号是在软件或硬件事件发生时由操作系统发送给进程的中断通知。

`signal()`的原型如下：

```c
#include <signal.h>

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);
```

参数说明：

- `signum`：要捕获和处理的信号编号。
- `handler`：指向信号处理函数的指针，用于处理接收到的信号。

`signal()`函数返回值是一个指向之前信号处理函数的指针，如果调用失败，则返回`SIG_ERR`。

下面是一个简单的示例，演示如何使用`signal()`函数设置信号处理函数：

```c
#include <stdio.h>
#include <signal.h>

void signalHandler(int signum) {
    printf("Received signal: %d\n", signum);
}

int main() {
    signal(SIGINT, signalHandler); // 设置对SIGINT信号的处理函数

    while (1) {
        // 主循环
    }

    return 0;
}
```

在上面的示例中，我们定义了一个名为`signalHandler`的信号处理函数，它接受一个整数参数表示接收到的信号编号。然后，我们使用`signal()`函数将`signalHandler`函数与`SIGINT`信号关联起来，即当接收到`SIGINT`信号（通常是通过按下Ctrl+C来触发）时，将调用`signalHandler`函数进行处理。

需要注意的是，`signal()`函数的用法在不同的操作系统和实现中可能会有所不同。因此，更加可移植的方法是使用更现代的信号处理函数接口，如`sigaction()`函数。

另外，信号处理函数应该尽量保持简短和非阻塞，以确保在处理信号时不会发生不可预测的行为。在信号处理函数中，应尽量避免调用不可重入的函数，因为信号处理函数可能在任何时间点被中断并再次调用。

还需要注意的是，某些信号具有默认的处理行为，例如`SIGINT`通常会导致程序终止。在使用`signal()`函数设置信号处理函数时，可以通过将处理函数设置为`SIG_IGN`来忽略信号，或者将其设置为`SIG_DFL`以恢复默认的处理行为。

#### 发送信号 `kill`

`kill()`用于向指定的进程发送信号。信号是由操作系统发送给进程的中断通知，用于通知进程发生了某个事件或请求进程执行某个操作。

`kill()`的原型如下：

```c
#include <sys/types.h>
#include <signal.h>

int kill(pid_t pid, int sig);
```

参数说明：

- `pid`：要发送信号的目标进程的进程ID。
- `sig`：要发送的信号编号。

`kill()`函数返回值为0表示操作成功，返回-1表示操作失败。

下面是一个简单的示例，演示如何使用`kill()`函数向指定进程发送信号：

```c
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

int main() {
    pid_t pid = 1234; // 目标进程的进程ID
    int sig = SIGTERM; // 要发送的信号编号

    if (kill(pid, sig) == -1) {
        perror("kill");
        return 1;
    }

    printf("Signal sent to process with PID %d\n", pid);

    return 0;
}
```

在上面的示例中，我们使用`kill()`函数向进程ID为`pid`的进程发送信号`sig`。如果`kill()`函数调用成功，将打印一条消息表示信号已经发送给目标进程。

需要注意的是，`kill()`函数可以用于向任何进程发送信号，但要发送信号给其他用户的进程，需要相应的权限。常见的信号包括`SIGTERM`（终止进程）、`SIGINT`（中断进程，通常由Ctrl+C触发）和`SIGKILL`（强制终止进程）等。

此外，如果`pid`参数为负值，`kill()`函数的行为会有所不同：

- 如果`pid`为负值且绝对值不为1，则信号将发送给进程组ID为`pid`绝对值的所有进程。
- 如果`pid`为-1，则信号将发送给除发送进程外的系统中所有进程。
- 如果`pid`为0，则信号将发送给与发送进程在同一进程组中的所有进程。
- 如果`pid`为正值，则信号将发送给进程ID为`pid`的进程。

需要谨慎使用`kill()`函数，因为发送错误的信号或错误的进程ID可能会导致意外的结果，甚至使系统不稳定。

#### 等待信号 `pause`

`pause()`用于使当前进程暂停执行，直到接收到一个信号。它将进程置于睡眠状态，直到接收到一个信号才会被唤醒。

`pause()`的原型如下：

```c
#include <unistd.h>

int pause(void);
```

`pause()`函数没有参数，也没有返回值。当进程调用`pause()`函数时，它会挂起自己的执行，进入睡眠状态，直到接收到一个信号。

下面是一个简单的示例，演示如何使用`pause()`函数使进程暂停执行，直到接收到`SIGINT`信号（通常由按下Ctrl+C触发）：

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void signalHandler(int signum) {
    printf("Received signal: %d\n", signum);
}

int main() {
    signal(SIGINT, signalHandler); // 设置对SIGINT信号的处理函数

    printf("Waiting for signal...\n");
    pause(); // 暂停进程执行，直到接收到信号

    printf("Resumed execution\n");

    return 0;
}
```

在上面的示例中，我们首先定义了一个名为`signalHandler`的信号处理函数，用于处理接收到的信号。然后，我们使用`signal()`函数将`signalHandler`函数与`SIGINT`信号关联起来。

在`main()`函数中，我们打印一条消息表示进程正在等待信号。然后，调用`pause()`函数将进程置于睡眠状态，直到接收到信号。当接收到`SIGINT`信号时，将调用`signalHandler`函数进行处理。

需要注意的是，`pause()`函数返回时，表示接收到一个信号并已经被处理。如果在调用`pause()`之前已经有信号到达，则`pause()`函数会立即返回，而不会挂起进程的执行。

`pause()`函数在某些情况下可以用于实现简单的同步机制，如等待其他进程发送信号来通知当前进程继续执行。然而，它通常被更高级的同步机制如条件变量、互斥锁等所替代，以提供更复杂的线程同步和通信功能。

#### 定时器 `alarm`

`alarm()`用于设置定时器，以在指定时间后发送`SIGALRM`信号给调用进程。它可以用于实现定时操作或在一定时间间隔内执行某个任务。

`alarm()`的原型如下：

```c
#include <unistd.h>

unsigned int alarm(unsigned int seconds);
```

参数说明：

- `seconds`：要设置的定时器的时间，以秒为单位。

`alarm()`函数返回值为之前设置的定时器的剩余时间。如果之前没有设置定时器，则返回0。

下面是一个简单的示例，演示如何使用`alarm()`函数在5秒后发送`SIGALRM`信号给当前进程：

```c
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void signalHandler(int signum) {
    printf("Received signal: %d\n", signum);
}

int main() {
    signal(SIGALRM, signalHandler); // 设置对SIGALRM信号的处理函数

    printf("Setting alarm for 5 seconds...\n");
    unsigned int remainingTime = alarm(5); // 设置5秒的定时器

    printf("Waiting for alarm...\n");
    pause(); // 等待SIGALRM信号

    printf("Alarm received\n");
    if (remainingTime > 0) {
        printf("Remaining time: %u seconds\n", remainingTime);
    }

    return 0;
}
```

在上面的示例中，我们首先定义了一个名为`signalHandler`的信号处理函数，用于处理接收到的信号。然后，我们使用`signal()`函数将`signalHandler`函数与`SIGALRM`信号关联起来。

在`main()`函数中，我们打印一条消息表示正在设置定时器。然后，调用`alarm(5)`函数设置一个5秒钟的定时器。接下来，我们打印一条消息表示正在等待定时器触发，然后调用`pause()`函数挂起进程的执行，直到接收到`SIGALRM`信号。

当接收到`SIGALRM`信号时，将调用`signalHandler`函数进行处理。在处理函数中，我们打印一条消息表示接收到了信号。如果`alarm()`函数之前设置了一个定时器，我们通过获取返回值来获取剩余时间，并将其打印出来。

需要注意的是，`alarm()`函数只能设置一个全局的定时器，所以在使用时需要小心，避免影响其他部分的代码。另外，如果在调用`alarm()`之前已经设置了一个定时器，则之前的定时器会被新的定时器取代，并返回之前定时器的剩余时间。

在实际应用中，如果需要更复杂的定时功能，可以使用更高级的定时器机制，如`timer_create()`和`timer_settime()`函数。这些函数提供更灵活的定时功能，包括周期性定时、精确控制定时器的触发时间等。

#### 定时器 `setitimer` 、`getitimer`

`setitimer()`和`getitimer()`是两个系统调用，用于设置和获取定时器的值。它们提供了更高级的定时功能，相比于`alarm()`函数，可以设置周期性定时器和获取定时器的剩余时间。

`setitimer()`的原型如下：

```c
#include <sys/time.h>

int setitimer(int which, const struct itimerval *new_value,
              struct itimerval *old_value);
```

`which`参数指定要设置的定时器类型，可以是以下值之一：

- `ITIMER_REAL`：真实时间定时器，以系统的实时时间衡量。
- `ITIMER_VIRTUAL`：虚拟时间定时器，只在进程执行用户代码时计时。
- `ITIMER_PROF`：以进程在用户代码和内核代码中所花费的时间之和作为定时器的衡量标准。

`new_value`参数指向一个`struct itimerval`结构体，用于设置新的定时器值。`struct itimerval`结构体包括两个成员：

- `it_value`：定时器的初始值，为0表示立即触发定时器。
- `it_interval`：定时器的重复间隔，为0表示只触发一次，大于0表示周期性触发。

`old_value`参数是一个可选的输出参数，用于获取之前的定时器值。

`setitimer()`函数返回值为0表示操作成功，返回-1表示操作失败。

下面是一个示例，演示如何使用`setitimer()`函数设置一个周期性定时器，每隔1秒触发一次：

```c
#include <stdio.h>
#include <sys/time.h>

void timerHandler(int signum) {
    printf("Timer triggered\n");
}

int main() {
    struct itimerval timer;
    struct timeval interval;

    // 设置定时器的初始值和重复间隔
    interval.tv_sec = 1;
    interval.tv_usec = 0;
    timer.it_interval = interval;
    timer.it_value = interval;

    // 设置定时器
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }

    // 设置定时器触发时的处理函数
    signal(SIGALRM, timerHandler);

    // 等待定时器触发
    while (1) {
    }

    return 0;
}
```

在上面的示例中，我们首先定义了一个名为`timerHandler`的信号处理函数，用于处理定时器触发时的操作。然后，我们创建一个`struct itimerval`结构体，并设置定时器的初始值和重复间隔为1秒。接下来，调用`setitimer()`函数将定时器设置为真实时间定时器，每隔1秒触发一次。

在`main()`函数中，我们使用`signal()`函数将`timerHandler`函数与`SIGALRM`信号关联起来，以便在定时器触发时执行相应的操作。最后，我们使用一个无限循环来等待定时器触发，以保持进程的执行。

`getitimer()`的原型如下：

```c
#include <sys/time.h>

int getitimer(int which, struct itimerval *curr_value);
```

`getitimer()`函数用于获取指定定时器的当前值。`which`参数指定要获取的定时器类型，可以是`ITIMER_REAL`、`ITIMER_VIRTUAL`或`ITIMER_PROF`。`curr_value`参数指向一个`struct itimerval`结构体，用于存储获取到的定时器值。

`getitimer()`函数返回值为0表示操作成功，返回-1表示操作失败。

下面是一个示例，演示如何使用`getitimer()`函数获取真实时间定时器的剩余时间：

```c
#include <stdio.h>
#include <sys/time.h>

int main() {
    struct itimerval timer;

    // 获取当前真实时间定时器的值
    if (getitimer(ITIMER_REAL, &timer) == -1) {
        perror("getitimer");
        return 1;
    }

    // 打印剩余时间
    printf("Remaining time: %ld seconds, %ld microseconds\n",
           timer.it_value.tv_sec, timer.it_value.tv_usec);

    return 0;
}
```

在上面的示例中，我们创建一个`struct itimerval`结构体，并调用`getitimer()`函数来获取当前真实时间定时器的值。然后，我们打印定时器的剩余时间。

需要注意的是，`setitimer()`和`getitimer()`函数在操作定时器时需要小心，以避免对其他部分的代码产生意外的影响。此外，这些函数在多线程环境中使用时需要考虑同步问题，以确保正确地设置和获取定时器的值。

### 信号通信实验

使用系统调用 `fork()` 函数创建两个子进程，再用系统调用 `signal()` 函数让父进程捕捉键盘上来的中断信号(即按 Del 键)，当父进程接收到这两个软中断的其中某一个后，父进程用系统调用 `kill()` 向两个子进程分别发送整数值为 16 和 17 软中断信号，子进程获得对应软中断信号后，分别输出下列信息后终止。

```txt
Child process 1 is killed by parent!!
Child process 2 is killed by parent!!
```

父进程调用 `wait()` 函数等待两个子进程终止后，输出以下信息后终止。

```txt
Parent process is killed! !
```

#### 程序通信流程图

![Imgur](https://i.imgur.com/Kve9YCg.png)

#### 程序代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <wait.h>

int wait_flag;

void stop() {}

int main()
{
    int pid1, pid2;
    signal(3, stop);

    while ((pid1 = fork()) == -1)
        ;

    if (pid1 > 0) {
        while ((pid2 = fork()) == -1)
            ;
        
        if (pid2 > 0) {
            wait_flag = 1;
            sleep(5);
            kill(pid1, 16);
            kill(pid2, 17);
            wait(0);
            wait(0);
            printf("\nParent process is killed!!\n");
            exit(0);
        } else {
            wait_flag = 1;
            signal(17, stop);
            printf("\nChild process 2 is killed by parent!!");
            exit(0);
        }
    } else {
        wait_flag = 1;
        signal(16, stop);
        printf("\nChild process 1 is killed by parent!!");
        exit(0);
    }

    return 0;
}
```

#### 运行结果及分析

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [10:16:55] C:1
$ gcc -o signal signal.c

# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [10:17:56] 
$ ./signal    

Child process 2 is killed by parent!!
Child process 1 is killed by parent!!
Parent process is killed!!
```

或者(运行多次后会出现如下结果)：

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab3 on git:main x [10:18:05] 
$ ./signal

Child process 1 is killed by parent!!
Child process 2 is killed by parent!!
Parent process is killed!!
```

简要分析：

(1) 上述程序中，调用函数 `signal()` 都放在一段程序的前面部分，而不是在其他接收信号处，这是因为 `signal()` 的执行只是为进程指定信号量 16 和 17 的作用，以及分配相应的与 `stop()` 过程链接的指针。因而 `signal()` 函数必须在程序前面部分执行。

(2) 该程序段前面部分用了两个 `wait(0)` ，这是因为父进程必须等待两个子进程终止后才终止。`wait() `函数常用来控制父进程与子进程的同步。在父进程中调用 `wait()` 函数，则父进程被阻塞。进入等待队列， 等待子进程结束。当子进程结束时，会产生一个终止状态字，系统会向父进程发出 `SIGCHLD` 信号。当接收到信号后，父进程提取子进程的终止状态字，从 `wait()` 函数返回继续执行原程序。

(3) 该程序中每个进程退出时都用了语句 `exit(0)`，这是进程的正常终止。在正常终止时，`exit()` 函数返回进程结束状态。异常终止时，由系统内核产生一个代表异常终止原因的终止状态，该进程的父进程都能用 `wait()` 函数得到其终止状态。在子进程调用 `exit()` 函效后，子进程的结束状态会返回给系统内核，由内核根据状态字生成终止状态，供父进程在 `wait()` 函数中读取数据。若子进程结束后，父进程还没有读取子进程的终止状态，则系统将于进程的终止状态置为“ZOMBIE”并保留子进程的进程控制块等信息，等父进程读取信息后，系统才彻底释放子进程的进程控制块。若父进程在子进程结束之前就结束，则子进程变“孤儿进程”，系统进程 init 会自动“收养”该子进程，成为该子进程的父进程，即父进程标识号变为 1，当子进程结束时，init 会自动调用 `wait()` 函数读取子进程的遗留数据，从而避免系统中留下大量的垃圾。

(4) 上述结果中 "Child process 1 is killed by parent!" 和 "Child process 2 is killed by parent!" 的出现，当运行几次后，谁在前谁在后是随机的，这是因为从进程调度的角度看，子进程被创建后处于就绪态，此时，父讲程和子进程作为两个独立的进程，共享同一个代码段，参加调度、执行直至进程结束，但是谁会先得到调度，与系统的调度策略和系统当前的资源状态有关，是不确定的，因此，谁先从 `fork()` 函数中返回继续执行后面的语句也是不确定的。