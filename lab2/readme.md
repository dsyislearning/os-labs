# lab2 线程创建管理与通信

## 实验环境

- OS: Arch Linux

- Kernel Version：6.5.9-arch2-1 (64-bit)

## 2-1 线程创建与管理

### Pthread 线程库 API

线程也称为轻质进程、轻量级进程（Light-Weight Process, LWP）。线程是多线程程序运行中的基本调度单位，是进程中一个单一顺序的控制流，一个进程内部可以有很多线程。同一进程中的多个线程共享分配给该进程的系统资源，比如文件描述符和信号处理等。

Linux 系统中的多线程遵循 POSIX 线程接口标准，称为 pthread，通过 Pthread 线程库来实现线程管理。编写 Linux 下的多线程程序，需要使用头文件 `pthread.h` ，链接时需要使用库 `libpthread.a` 。

Linux 下 pthread 的实现是通过系统调用 `clone()` 来实现的，`clone()` 是 Linux 所特有的系统调用，其使用方式类似 `fork()`。

线程与进程比较：

- 在 Linux 系统中，启动一个新的进程必须分配给它独立的地址空间，建立数据结构以维护其代码段、堆栈段和数据段，管理代价非常“昂贵”。而运行于一个进程中的多个线程，它们彼此之间使用相同的地址空间，共享大部分数据，启动一个线程所花费的空间远远小于启动一个进程所花费的空间，而且线程间彼此切换所需要时间也远远小于进程间切换所需要的时间。
- 线程间方便的通信机制。对不同进程来说它们具有独立的数据空间，要进行数据的传递只能通过通信的方式进行，费时且不方便。线程则不然，由于同一进程下的线程之间共享数据空间，所以一个线程的数据可以直接为其他线程所用，方便快捷。

Pthread 线程库提供的基本线程管理函数有：

- 创建线程 `pthread_create`
- 线程退出 `pthread_exit`
- 线程脱离 `pthread_detach`
- 线程取消 `pthread_cancel`
- 线程清除 `pthread_cleanup_push/pthread_cleanup_pop`
- 线程等待 `pthread_join`
- 线程标识获取 `pthread_self`

#### 线程创建  `pthread_create`

`pthread_create` 函数用于创建新的线程。它允许在程序中同时执行多个独立的线程，并行处理不同的任务。

`pthread_create` 函数的声明如下：

```c
#include <pthread.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg);
```

下面是参数的解释：

- `thread`：指向 `pthread_t` 类型的指针，用于存储新线程的标识符。
- `attr`：指向 `pthread_attr_t` 类型的指针，用于指定新线程的属性。可以为 `NULL`，表示使用默认属性。
- `start_routine`：指向线程函数的指针，该线程函数在新线程中执行。线程函数的原型为 `void* function_name(void* arg)`，其中 `arg` 是传递给线程函数的参数。
- `arg`：传递给线程函数的参数。

`pthread_create` 函数的返回值是一个整数，用于指示函数的执行结果。

- 如果创建线程成功，返回值为 0；
- 如果出现错误，返回值为一个非零的错误代码。

以下是一个简单的示例，展示了如何使用 `pthread_create` 创建一个新线程：

```c
#include <pthread.h>
#include <stdio.h>

void* thread_function(void* arg) {
    int thread_id = *(int*)arg;
    printf("This is thread %d\n", thread_id);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    int thread_id = 1;

    int result = pthread_create(&thread, NULL, thread_function, &thread_id);
    if (result != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    // 等待线程结束
    pthread_join(thread, NULL);

    return 0;
}
```

在这个示例中，我们定义了一个名为 `thread_function` 的线程函数，它接收一个整数参数作为线程的标识符，并在标准输出中打印出线程的编号。然后，在 `main` 函数中，我们创建了一个新线程，并传递了线程函数的地址以及一个指向整数的指针作为参数。

`pthread_join` 函数用于等待线程的结束。通过调用 `pthread_join(thread, NULL)`，主线程将等待新线程执行完毕后再继续执行。

请注意，在编译时需要链接 pthread 库，例如使用 `-pthread` 选项。

#### 线程退出 `pthread_exit`

`pthread_exit` 是一个函数，用于在线程中显式地退出线程的执行。当线程完成了它的任务或需要提前终止时，可以调用 `pthread_exit` 来终止线程的执行并返回一个指定的退出状态。

`pthread_exit` 的声明如下：

```c
#include <pthread.h>

void pthread_exit(void *value_ptr);
```

参数 `value_ptr` 是一个指针，用于指定线程的退出状态。该指针可以传递一个特定的值，以便在主线程或其他线程中获取线程的退出状态。通常，可以使用 `NULL` 表示不需要返回任何状态。

当线程调用 `pthread_exit` 时，它会立即终止线程的执行，不会执行之后的代码。线程的资源会被自动释放，包括堆栈和其他与线程相关的资源。同时，线程的退出状态也会被传递给等待该线程结束的其他线程。

下面是一个示例，展示了如何在线程中使用 `pthread_exit`：

```c
#include <pthread.h>
#include <stdio.h>

void* thread_function(void* arg) {
    int thread_id = *(int*)arg;
    printf("This is thread %d\n", thread_id);

    // 线程执行完毕，退出并返回状态
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    int thread_id = 1;

    int result = pthread_create(&thread, NULL, thread_function, &thread_id);
    if (result != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    // 等待线程结束
    pthread_join(thread, NULL);

    printf("Thread has exited\n");

    return 0;
}
```

在这个示例中，线程函数 `thread_function` 接收一个整数参数作为线程的标识符，在标准输出中打印出线程的编号，然后调用 `pthread_exit` 来终止线程的执行。

`pthread_join` 函数用于等待线程的结束。通过调用 `pthread_join(thread, NULL)`，主线程将等待新线程执行完毕后再继续执行。

需要注意的是，`pthread_exit` 只会终止当前线程的执行，并不会影响其他正在运行的线程。如果希望终止整个进程，可以在主线程中调用 `exit` 函数。

#### 线程脱离 `pthread_detach`

`pthread_detach` 是一个函数，用于将线程标记为“脱离状态”（detached state）。当一个线程被标记为脱离状态后，它的资源将在线程退出时自动释放，而不需要其他线程调用 `pthread_join` 来等待它的结束。

`pthread_detach` 的声明如下：

```c
#include <pthread.h>

int pthread_detach(pthread_t thread);
```

参数 `thread` 是一个 `pthread_t` 类型的线程标识符，用于指定要脱离的线程。

调用 `pthread_detach` 后，线程将进入脱离状态，线程的资源将在线程退出时自动释放，而无需其他线程等待。脱离状态的线程无法被其他线程等待或获取其退出状态。

下面是一个示例，展示了如何使用 `pthread_detach` 来脱离线程：

```c
#include <pthread.h>
#include <stdio.h>

void* thread_function(void* arg) {
    printf("This is a detached thread\n");

    // 线程执行完毕，自动释放资源
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;

    int result = pthread_create(&thread, NULL, thread_function, NULL);
    if (result != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    // 将线程标记为脱离状态
    result = pthread_detach(thread);
    if (result != 0) {
        printf("Failed to detach thread\n");
        return 1;
    }

    printf("Thread has been detached\n");

    // 继续执行其他任务...

    return 0;
}
```

在这个示例中，我们创建了一个新线程，并在 `main` 函数中使用 `pthread_detach` 将该线程标记为脱离状态。然后，主线程继续执行其他任务，而不需要等待新线程的结束。

需要注意的是，脱离状态的线程一旦被标记为脱离，就无法再将其恢复为可等待状态。因此，在调用 `pthread_detach` 之前，确保不需要等待该线程的结束。

#### 线程取消 `pthread_cancel`

`pthread_cancel` 是一个函数，用于取消指定的线程。当调用 `pthread_cancel` 时，目标线程将收到一个取消请求，可以在适当的时候响应该请求并终止自己的执行。

`pthread_cancel` 的声明如下：

```c
#include <pthread.h>

int pthread_cancel(pthread_t thread);
```

参数 `thread` 是一个 `pthread_t` 类型的线程标识符，用于指定要取消的线程。

调用 `pthread_cancel` 后，目标线程将收到一个取消请求，但并不保证它会立即终止执行。线程可以选择在适当的时候响应取消请求并退出，也可以忽略取消请求继续执行。

需要注意的是，线程只能在特定的取消点（cancellation point）才能响应取消请求。取消点是指在这些点上线程会检查是否有取消请求，如果有，则执行相应的取消处理操作。常见的取消点包括线程库函数调用、I/O 操作、阻塞的系统调用等。

下面是一个示例，展示了如何使用 `pthread_cancel` 来取消线程：

```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h> // sleep 函数

void* thread_function(void* arg) {
    printf("Thread started\n");

    // 设置取消点
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    // 执行一些工作
    int i;
    for (i = 0; i < 5; i++) {
        printf("Working %d\n", i);
        sleep(1);
    }

    printf("Thread finished\n");

    pthread_exit(NULL);
}

int main() {
    pthread_t thread;

    int result = pthread_create(&thread, NULL, thread_function, NULL);
    if (result != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    // 等待一段时间后取消线程
    sleep(3);
    result = pthread_cancel(thread);
    if (result != 0) {
        printf("Failed to cancel thread\n");
        return 1;
    }

    // 等待线程结束
    pthread_join(thread, NULL);

    printf("Thread has been canceled\n");

    return 0;
}
```

在这个示例中，我们创建了一个新线程，并在 `main` 函数中使用 `pthread_cancel` 来取消该线程。然后，主线程等待一段时间后调用 `pthread_cancel` 发送取消请求。

目标线程在执行过程中会打印一些工作信息，并在每次循环迭代之间使用 `sleep` 函数进行暂停。设置了取消点(在上述例子中是阻塞的系统调用 `sleep()`）后，线程会在每个取消点检查是否有取消请求。我们使用 `pthread_setcancelstate` 和 `pthread_setcanceltype` 来设置线程的取消状态和取消类型。

- `PTHREAD_CANCEL_ENABLE` 表示线程的取消状态被启用。当线程的取消状态为启用时，它可以接收取消请求并响应。取消状态可以是启用或禁用两种，启用状态下线程可以被取消，禁用状态下线程无法被取消。
- `PTHREAD_CANCEL_DEFERRED` 表示线程的取消类型为延迟取消。延迟取消类型意味着线程在接收到取消请求后，只有在达到取消点时才会实际响应取消。取消点是指线程会检查是否有取消请求的特定位置。如果线程的取消类型为延迟取消，它会继续执行直到达到取消点，然后在取消点处响应取消。

需要注意的是，线程取消是异步的，即主线程调用 `pthread_cancel` 后立即返回，而不会等待线程的终止。如果需要等待线程的终止，可以使用 `pthread_join`。

#### 线程清除 `pthread_cleanup_push/pthread_cleanup_pop`

`pthread_cleanup_push` 和 `pthread_cleanup_pop` 是一对函数，用于管理线程清理处理函数（thread cleanup handlers）。线程清理处理函数是在线程退出时自动执行的函数，用于清理线程执行期间分配的资源或执行其他必要的清理操作。

这对函数的声明如下：

```c
#include <pthread.h>

void pthread_cleanup_push(void (*routine)(void *), void *arg);
void pthread_cleanup_pop(int execute);
```

- `pthread_cleanup_push` 用于将一个线程清理处理函数入栈。参数 `routine` 是一个函数指针，指向要执行的清理处理函数。参数 `arg` 是传递给清理处理函数的参数。
- `pthread_cleanup_pop` 用于将栈顶的线程清理处理函数出栈，并根据 `execute` 参数决定是否执行该清理处理函数。`execute` 参数为非零值时，清理处理函数将被执行；为零时，清理处理函数将被丢弃而不执行。

下面是一个示例，展示了如何使用 `pthread_cleanup_push` 和 `pthread_cleanup_pop` 来管理线程清理处理函数：

```c
#include <pthread.h>
#include <stdio.h>

void cleanup_function(void *arg) {
    printf("Cleanup function executed\n");
    // 清理资源或执行其他清理操作
}

void* thread_function(void* arg) {
    printf("Thread started\n");

    // 将清理处理函数入栈
    pthread_cleanup_push(cleanup_function, NULL);

    // 线程执行一些工作
    printf("Working...\n");

    // 弹出清理处理函数并执行
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

int main() {
    pthread_t thread;

    int result = pthread_create(&thread, NULL, thread_function, NULL);
    if (result != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    // 等待线程结束
    pthread_join(thread, NULL);

    printf("Thread finished\n");

    return 0;
}
```

在这个示例中，我们创建了一个新线程，并在线程函数中使用 `pthread_cleanup_push` 和 `pthread_cleanup_pop` 来管理清理处理函数。

线程函数中的 `pthread_cleanup_push(cleanup_function, NULL)` 将 `cleanup_function` 入栈作为清理处理函数。这意味着在线程退出时，将执行该清理函数。

紧接着，在清理处理函数入栈后，线程执行一些工作。

最后，通过 `pthread_cleanup_pop(1)` 弹出清理处理函数并执行。参数 `1` 表示执行清理处理函数。如果参数为 `0`，则清理处理函数会被丢弃而不执行。

需要注意的是，清理处理函数的执行是在 `pthread_cleanup_pop` 被调用时确定的，而不是在线程被取消或正常退出时。每个 `pthread_cleanup_push` 必须与一个对应的 `pthread_cleanup_pop` 配对使用，以确保栈的正确管理。

线程清理处理函数可以用于释放动态分配的内存、关闭打开的文件描述符、释放其他资源等。通过使用线程清理处理函数，可以确保在线程退出时进行必要的清理操作，以避免资源泄漏和其他问题。

线程终止有两种情况：正常终止和非正常终止。线程主动调用 `pthread_exit()` 或者从线程函数中 return 都将使线程正常退出，属于正常的、可预见的退出方式；如果线程在其它线程的干预下，或者由于自身运行出错（例如访问非法地址）而退出，对线程自身而言，这种退出方式是非正常、不可预见的。

对线程可预见的正常终止和不可预见异常终止，通过两个线程清除函数，可以保证线程终止时能顺利地释放掉自己所占用的资源。

在线程函数体内先后调用 `pthread_cleanup_push()` 、`pthread_cleanup_pop()` 后，在从 `pthread_cleanup_push()` 的调用点到 `pthread_cleanup_pop()` 之间的程序段中的各种线程终止操作（包括 `pthread_exit()` 和异常终止，但不包括 return）都将执行 `pthread_cleanup_push()` 所指定的清理函数。

#### 线程等待 `pthread_join()`

`pthread_join` 是一个线程函数，用于等待指定的线程完成执行，并获取线程的返回值。

函数声明如下：

```c
#include <pthread.h>

int pthread_join(pthread_t thread, void **retval);
```

- `thread` 是要等待的目标线程的标识符，通常使用 `pthread_t` 类型的变量表示。
- `retval` 是一个指向指针的指针，用于存储目标线程的返回值。目标线程的返回值是通过 `pthread_exit` 函数设置的，如果不需要获取返回值，可以将 `retval` 参数设置为 `NULL`。

`pthread_join` 函数的作用是阻塞当前线程，直到指定的目标线程完成执行。如果目标线程已经完成执行，那么 `pthread_join` 函数立即返回；否则，当前线程将一直等待，直到目标线程完成为止。

当目标线程完成执行后，它的返回值（如果有）可以通过 `retval` 参数获取。`retval` 参数应该是一个指向指针的指针，因为线程的返回值是一个 `void*` 类型的指针。通过将 `retval` 参数设置为指向合适的内存位置，就可以将目标线程的返回值传递给当前线程。

以下是一个示例，展示了如何使用 `pthread_join` 函数等待线程完成并获取其返回值：

```c
#include <pthread.h>
#include <stdio.h>

void* thread_function(void* arg) {
    int value = 42;
    pthread_exit((void*)value);
}

int main() {
    pthread_t thread;
    int result;

    result = pthread_create(&thread, NULL, thread_function, NULL);
    if (result != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    void* thread_result;
    result = pthread_join(thread, &thread_result);
    if (result != 0) {
        printf("Failed to join thread\n");
        return 1;
    }

    printf("Thread returned: %d\n", (int)thread_result);

    return 0;
}
```

在这个示例中，我们创建了一个新线程，并在线程函数中通过 `pthread_exit` 设置了返回值为 `42`。在主线程中，我们使用 `pthread_join` 等待线程完成，并通过 `&thread_result` 参数获取线程的返回值。

最终，我们将线程的返回值打印出来。

需要注意的是，`pthread_join` 函数只能等待一个线程完成执行。如果需要等待多个线程完成，可以多次调用 `pthread_join`。

另外，如果目标线程被取消，或者目标线程已经调用了 `pthread_exit`，那么 `pthread_join` 函数也会立即返回。

`pthread_join` 函数对于线程的同步和协作非常有用，可以确保线程的执行顺序以及获取线程的返回值。在使用 `pthread_join` 函数时，请注意处理可能的错误返回值，并确保正确处理线程的返回值类型。

一个进程内部的多个线程共享进程内的数据段。当进程内一个线程退出后，退出线程所占用的资源并不会随着该线程的终止而得到释放。类似于进程之间使用 `wait()`系统调用来同步终止并释放资源，线程之间 `pthread_join()` 函数实现线程同步和资源释放。

`pthread_join()` 将当前进程挂起来等待线程的结束。这个函数是一个线程阻塞的函数，调用它的函数将一直等待到被等待的线程结束为止，当函数返回时，被等待线程的资源被收回。

#### 线程标识获取 `pthread_self()`

`pthread_self` 是一个线程函数，用于获取当前线程的标识符。

函数声明如下：

```c
#include <pthread.h>

pthread_t pthread_self(void);
```

`pthread_self` 函数不需要任何参数，它会返回当前线程的标识符，类型为 `pthread_t`。

每个线程都有一个唯一的标识符，可以通过 `pthread_self` 函数获取。线程标识符是一个不透明的数据类型，通常是一个结构体或指针，用于在编程中表示线程。

以下是一个示例，展示了如何使用 `pthread_self` 函数获取当前线程的标识符：

```c
#include <pthread.h>
#include <stdio.h>

void* thread_function(void* arg) {
    pthread_t thread_id = pthread_self();
    printf("Thread ID: %lu\n", thread_id);
    return NULL;
}

int main() {
    pthread_t thread;

    int result = pthread_create(&thread, NULL, thread_function, NULL);
    if (result != 0) {
        printf("Failed to create thread\n");
        return 1;
    }

    pthread_t main_thread_id = pthread_self();
    printf("Main Thread ID: %lu\n", main_thread_id);

    // 等待线程结束
    pthread_join(thread, NULL);

    return 0;
}
```

在这个示例中，我们创建了一个新线程，并在线程函数中通过 `pthread_self` 获取当前线程的标识符。我们还在主线程中获取了主线程的标识符。

最终，我们将线程的标识符打印出来。

需要注意的是，线程标识符的具体类型和打印格式可能因操作系统和编译器而异。在这个示例中，我们使用 `%lu` 打印格式来打印无符号长整型值。

`pthread_self` 函数对于需要在线程内部获取自身标识符的情况非常有用，例如在线程函数中记录日志、识别线程或进行其他与线程标识相关的操作。

需要注意的是，线程标识符只在创建线程的进程中有效，不同进程中的线程标识符可能不同。另外，不同线程之间的标识符可能会重叠，因此不能通过直接比较线程标识符来判断线程的唯一性，而应该使用其他线程同步和通信机制。

## 2-2 线程通信

属于同一个进程中的多个线程使用相同的地址空间，共享大部分数据，一个线程的数据可以直接为其它线程所用，这些线程相互间可以方便快捷地利用共享数据结构进行通信。

线程间通信分为两种情况：

1. 分属于不同进程的线程间的通信。可以采用进程间通信机制，如消息队列；
2. 位于同一进程内的不同线程间的通信。此时，线程可以通过共享的进程全局数据结构进行快速通信，但需要考虑采用合适的同步互斥机制，以保证访问结果的正确，这类似于进程间的同步与互斥。

一般认为，Linux 系统中，线程间通信方式包括：

- 锁机制：包括互斥锁、条件变量、读写锁和自旋锁。

  互斥锁确保同一时间只能有一个线程访问共享资源。当锁被占用时试图对其加锁的线程都进入阻塞状态（释放 CPU 资源使其由运行状态进入等待状态）。当锁释放时哪个等待线程能获得该锁取决于内核的调度。

  读写锁当以写模式加锁而处于写状态时任何试图加锁的线程（不论是读或写）都阻塞，当以读状态模式加锁而处于读状态时“读”线程不阻塞，“写”线程阻塞。读模式共享，写模式互斥。

  条件变量以原子的方式阻塞进程，直到某个特定条件为真为止。对条件的测试是在互斥锁的保护下进行的。条件变量始终与互斥锁一起使用。

  自旋锁上锁受阻时线程不阻塞而是在循环中轮询查看能否获得该锁，没有线程的切换因而没有切换开销，不过对 CPU 的霸占会导致 CPU 资源的浪费。 所以自旋锁适用于并行结构（多个处理器）或者适用于锁被持有时间短而不希望在线程切换产生开销的情况。

- 信号量机制（Semaphore）：包括无名线程信号量和命名线程信号量

- 信号机制（Signal）：类似进程间的信号处理

在本课程实验中，对线程间的通信，考虑一种特殊情况，即线程间参数传递。

### Pthread 线程间参数传递

#### 向新的线程传递整型值

样例代码为 `lab2/pass_int.c` ，内容如下：

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *run(void *arg)
{
    int *num = (int*) arg;
    printf("Create parameter is %d\n", *num);
    return (void *)0;
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    int error;

    int test = 4;
    int *test_ptr = &test;

    error = pthread_create(&tid, NULL, run, (void *)test_ptr);
    if (error != 0) {
        perror("pthread_create");
        return -1;
    }
    sleep(1);

    printf("Pthread_create is created...\n");

    return 0;
}
```

这段 C 代码展示了如何使用 POSIX 线程（pthreads）库创建一个新线程并向新线程传递整型值。程序的主函数创建了一个新线程，该线程执行 `run` 函数，并将一个整数作为参数传递给它。

`run` 函数被定义为接受一个 `void` 指针作为其参数。这在 C 语言中是一种常见的做法，当我们希望一个函数能够处理不同类型的数据时，就会这样做。在这种情况下，`run` 函数期望其参数是一个指向整数的指针。它将 `void` 指针强制转换回 `int` 指针，解引用它以获取整数值，然后打印它。

在 `main` 函数中，定义了一个整数 `test`，并将其地址存储在 `test_ptr` 指针中。然后，这个指针作为参数传递给 `pthread_create` 函数，该函数创建一个新线程，该线程开始执行 `run` 函数。`pthread_create` 函数接受四个参数：一个指向 `pthread_t` 的指针（它将保存创建的线程的ID），一个 `pthread_attr_t` 指针（可以用来设置线程属性 - 这里传递 `NULL` 以使用默认属性），新线程要执行的函数，以及要传递给该函数的参数。

`pthread_create` 函数在成功创建线程时返回 0。如果返回非零值，则发生错误。代码检查了这一点，如果线程创建失败，就打印错误消息。

创建线程后，主线程睡眠1秒。这可能是为了确保创建的线程有足够的时间执行，然后主线程继续。睡眠后，打印一条消息，表示线程已创建。

程序并没有明确等待创建的线程完成执行（使用`pthread_join`或类似的函数），所以如果主线程在创建的线程完成执行之前结束，整个进程将退出，可能会提前终止创建的线程。

编译运行样例代码，注意链接 pthread 动态链接库，结果如下：

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:01:54] 
$ gcc -o pass_int pass_int.c -lpthread 

# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:02:02] 
$ ./pass_int 
Create parameter is 4
Pthread_create is created...
```

从上可看出，在 main 函数中传递的 int 指针，传递到新建的线程函数中。

#### 向新线程传递字符串

样例代码为 `lab2/pass_string.c` ，内容如下：

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *run(void *arg)
{
    char *str = (char*) arg;
    printf("Create parameter is: %s\n", str);
    return (void *)0;
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    int error;

    char *test = "Hello!";

    error = pthread_create(&tid, NULL, run, (void *)test);
    if (error != 0) {
        perror("pthread_create");
        return -1;
    }
    sleep(1);

    printf("Pthread_create is created...\n");

    return 0;
}
```

只是将上一份代码传入的参数由整型改为了字符串类型，同时注意字符串类型本身由指针指示，也不再需要取地址和取内容。

编译运行样例代码，注意链接 pthread 动态链接库，结果如下：

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:13:58] 
$ gcc -o pass_string pass_string.c -lpthread

# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:14:05] 
$ ./pass_string                             
Create parameter is: Hello!
Pthread_create is created...
```

可看出，main 函数中的字符串传入到新建的线程中。

#### 向新线程传递结构体

样例代码为 `lab2/pass_struct.c` ，内容如下：

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

struct member {
    char *name;
    int age;
};

void *run(void *arg)
{
    struct member *p = (struct member*) arg;
    printf("member->name: %s\n", p->name);
    printf("member->age: %d\n", p->age);
    return (void *)0;
}

int main(int argc, char *argv[])
{
    pthread_t tid;
    int error;

    struct member *p = (struct member *)malloc(sizeof(struct member));
    p->name = "dsy";
    p->age = 20;

    error = pthread_create(&tid, NULL, run, (void *)p);
    if (error != 0) {
        perror("pthread_create");
        return -1;
    }
    sleep(1);

    printf("Pthread_create is created...\n");

    free(p);
    p = NULL;

    return 0;
}

```

同样也只是将前面代码传递的参数改为了结构体，按照无类型指针强制类型转换的操作就可以传递任意类型的参数。

编译运行样例代码，注意链接 pthread 动态链接库，结果如下：

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:14:07] 
$ gcc -o pass_struct pass_struct.c -lpthread

# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:24:03] 
$ ./pass_struct 
member->name: dsy
member->age: 20
Pthread_create is created...
```

可以看出，main 函数中的一个结构体传入了新建的线程中。 线程包含了标识进程内执行环境必须的信息，集成了进程中的所有信息，对线程进行共享的，包括文本程序、程序的全局内存和堆内存、栈以及文件描述符。

#### 验证新建线程可以共享进程中的数据

样例代码为 `lab2/share.c` ，内容如下：

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static int a = 5;

void *create(void *arg)
{
    printf("New pthread...\n");
    printf("a = %d\n",a);

    return (void *)0;
}

int main(int argc, const char *argv[])
{
    int error;

    pthread_t tid;

    error = pthread_create(&tid, NULL, create, NULL);

    if(error != 0) {
        printf("new thread is not created!\n");
        return -1;
    }
    sleep(1);

    printf("New thread is created...\n");

    return 0;
}
```

在原来进程的上下文中创建静态全局变量 a ，然后在新创建的线程中访问该变量。

编译运行样例代码，注意链接 pthread 动态链接库，结果如下：

```zsh
# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:30:45] 
$ gcc -o share share.c -lpthread

# dsy @ arch in ~/Code/bupt/os_labs/lab2 on git:main x [21:30:46] 
$ ./share                       
New pthread...
a = 5
New thread is created...
```

可以看出，在主线程更改全局变量 a 的值的时候，新建立的线程打印出改变后的值，可以看出可以访问线程所在进程中的数据信息。
