# lab4 进程/线程同步互斥

## 实验环境

- Machine：82NC Lenovo XiaoXinPro 14IHU 2021
- OS：Arch Linux
- Kernel Version：6.5.9-arch2-1 (64-bit)

## 4-1 生产流水线

### 问题描述

An assembly line is to produce a product $C$ with $n_1=4$ part $A$s, and $n_2=3$ part $B$s.

The worker of machining A and worker of machining B produce $m_1=2$ part $A$s and $m_2=1$ part $B$ independently each time.

Then the $m_1$ part $A$s or $m_2$ part $B$ will be moved to a station, which can hold at most $N=12$ of part $A$s and part $B$s altogether.

The produced $m_1$ part $A$s must be put onto the station **simultaneously**.

The workers must **exclusively** put a part on the station or get it from the station.

In addition, the worker to make $C$ must get **all** part of $A$s and $B$s, i.e. $n_1$ part $A$s and $n_2$ part $B$s, for one product $C$ once.

Using semaphores to coordinate the three workers who are machining part $A$, part $B$ and manufacturing the product $C$ without deadlock.

It is required that

1. definition and initial value of each semaphore, and
2. the algorithm to coordinate the production process for the three workers should be given.

### 原理分析

本题目是生产者-消费者问题的扩展，需要注意题目要求：

- worker A 一次生产 $m_1=2$ 个 A，必须一次性放入工作台
- worker C 必须一次性获得所需的 $n_1=4$ 个 A 和 $n_2=3$ 个 B。

换句话说，如果当前工作台空位小于 $m_1$ ，worker A 被阻塞；如果当前工作台没有足够的 A、B，worker C 被阻塞。

采用类似于多次 `wait(empty)` 的操作，为 worker A 从工作台获取多个空位是不合理的。

例如，假设当前工作台已经放置了 $N=11$ 个零件，只有 1 个空位。worker A 生产出 2 个 A，worker B 生产出 1 个 B。如果 worker A 先通过两次 `wait(empty)` 操作申请两个工作台空位，将被阻塞，生产的零件 A 没有放入工作台。随后 worker B 再执行 `wait(empty)` 申请空位也将被阻塞，而此时工作台有 1 个空位。

因此，正确的解决方案是：

1. 利用用户空间计数变量 `count_A`、 `count_B`、 `count_empty`，配合互斥信号量 `mutex_A`、 `mutex_B`、`mutex_empty`，统计工作台中零件 A、零件 B、空单元的数目，当有足够多的零件 A、B 和工作台空位时，再一次性申请/获取多个所需的零件 A、零件 B、工作台空位，即将 worker A 所需的 $m_1$ 个工作台空位作为一个整体一次性地申请，worker C 所需的工作台中的 $n_1$ 个 A 和 $n_2$ 个 B 作为一个整体一次性地申请；
2. 当所需工作台空位不满足时，worker A 和 worker B 应主动阻塞自身（suspend/block）， 防止忙等待。当 worker C 所需的工作台中 A 和 B 不满足时，也应主动阻塞自身；
3. 当 worker A、worker B 分别生产并放入新的零件 A、B 后，考虑唤醒由于所需零件不足而处于阻塞态的 worker C；同样地，当 worker C 从工作台取出零件 A、零件 B 后，考虑唤醒因没有足够空位而处于阻塞态的 worker A、worker B。

### 代码实现

我使用 POSIX 线程的 pthread 库进行多线程编程，完整代码位于 `lab4/assembly_line.c` 。

#### 信号量初值

由于 worker A、B、C 的工作条件都与对应的计数变量有关，发现不使用信号量只使B用互斥锁和条件判断也能够完成同步。

```c
#define N1 4
#define N2 3
#define M1 2
#define M2 1
#define N 12

int countA = 0, countB = 0, count_empty = N;
pthread_mutex_t mutex_A, mutex_B, mutex_empty;
```

信号量初始值如上所示，与原理分析中的变量含义一致。同时我使用宏来使得代码的可编辑性得到增强。

#### worker A/B

使用宏定义了 M1 和 M2 后可以将 A、B 的代码做到对称化，并且可以修改宏值得到通解。

需要注意的一点是需要在生产之前做好判断，不能让一种零件占满整个生产线，否则 worker C 无法同时拿到足够数量的 A 和 B 会出现死锁。

worker A：

```c
void * routine_A(void * arg)
{
    while (1) {
        // sleep(1);
        pthread_mutex_lock(&mutex_empty);
        if (count_empty >= M1) { // 如果有足够空位则生产，否则等待
            pthread_mutex_unlock(&mutex_empty);
            pthread_mutex_lock(&mutex_B);
            if (countB + count_empty - N1 >= M2) { // 如果A生产以后还能给B留足够空位则生产，否则等待
                pthread_mutex_unlock(&mutex_B);
                pthread_mutex_lock(&mutex_empty);
                count_empty -= M1;
                pthread_mutex_unlock(&mutex_empty);
                pthread_mutex_lock(&mutex_A);
                // sleep(M1);
                countA += M1;
                print_log("worker A put 2 part As on the station, %d part As on the station now.", countA);
                pthread_mutex_unlock(&mutex_A);
            } else {
                pthread_mutex_unlock(&mutex_B);
            }
        } else {
            pthread_mutex_unlock(&mutex_empty);
        }
    }
    pthread_exit(NULL);
}
```

worker B 与 worker A 对称：

```c
void * routine_B(void * arg)
{
    while (1) {
        // sleep(1);
        pthread_mutex_lock(&mutex_empty);
        if (count_empty >= M2) { // 如果有足够空位则生产，否则等待
            pthread_mutex_unlock(&mutex_empty);
            pthread_mutex_lock(&mutex_A);
            if (countA + count_empty - N2 >= M1) { // 如果B生产以后还能给A留足够空位则生产，否则等待
                pthread_mutex_unlock(&mutex_A);
                pthread_mutex_lock(&mutex_empty);
                count_empty -= M2;
                pthread_mutex_unlock(&mutex_empty);
                pthread_mutex_lock(&mutex_B);
                // sleep(M2);
                countB += M2;
                print_log("worker B put 1 part B on the station, %d part Bs on the station now.", countB);
                pthread_mutex_unlock(&mutex_B);
            } else {
                pthread_mutex_unlock(&mutex_A);
            }
        } else {
            pthread_mutex_unlock(&mutex_empty);
        }
    }
    pthread_exit(NULL);
}
```

#### worker C

判断变量之前先加锁，判断完之后立刻释放锁，同时注意判断真假不同分支下对锁的释放处理。

```c
void * routine_C(void * arg)
{
    while (1) {
        // sleep(1);
        pthread_mutex_lock(&mutex_A);
        if (countA >= N1) {
            pthread_mutex_unlock(&mutex_A);
            pthread_mutex_lock(&mutex_B);
            if (countB >= N2) {
                pthread_mutex_unlock(&mutex_B);
                pthread_mutex_lock(&mutex_A);
                // sleep(N1);
                countA -= N1;
                print_log("worker C took %d part A from the station, %d part As on the station now.", N1, countA);
                pthread_mutex_unlock(&mutex_A);
                pthread_mutex_lock(&mutex_B);
                // sleep(N2);
                countB -= N2;
                print_log("worker C took %d part B from the station, %d part Bs on the station now.", N2, countB);
                pthread_mutex_unlock(&mutex_B);
                pthread_mutex_lock(&mutex_empty);
                count_empty += N1 + N2;
                print_log("worker C took %d part A and %d part B from the station, %d empty on the station now.", N1, N2, count_empty);
                pthread_mutex_unlock(&mutex_empty);
            } else {
                pthread_mutex_unlock(&mutex_B);
            }
        } else {
            pthread_mutex_unlock(&mutex_A);
        }
    }
    pthread_exit(NULL);
}
```

### 测试结果

部分输出如下所示，可以看到 A 与 B 抢占式生产内容，当流水线满了以后 A 和 B 停止生产。

C 会在有足够 A 和 B 的条件同时满足时取出 A 和 B。

```
[2023-11-02 21:00:59.110808] worker B put 1 part B on the station, 2 part Bs on the station now.
[2023-11-02 21:00:59.110813] worker A put 2 part As on the station, 6 part As on the station now.
[2023-11-02 21:00:59.110822] worker A put 2 part As on the station, 8 part As on the station now.
[2023-11-02 21:00:59.110834] worker B put 1 part B on the station, 3 part Bs on the station now.
[2023-11-02 21:00:59.110844] worker B put 1 part B on the station, 4 part Bs on the station now.
[2023-11-02 21:00:59.110853] worker C took 4 part A from the station, 4 part As on the station now.
[2023-11-02 21:00:59.110858] worker C took 3 part B from the station, 1 part Bs on the station now.
[2023-11-02 21:00:59.110863] worker C took 4 part A and 3 part B from the station, 7 empty on the station now.
[2023-11-02 21:00:59.110872] worker B put 1 part B on the station, 2 part Bs on the station now.
[2023-11-02 21:00:59.110878] worker B put 1 part B on the station, 3 part Bs on the station now.
[2023-11-02 21:00:59.110883] worker B put 1 part B on the station, 4 part Bs on the station now.
[2023-11-02 21:00:59.110888] worker B put 1 part B on the station, 5 part Bs on the station now.
[2023-11-02 21:00:59.110893] worker B put 1 part B on the station, 6 part Bs on the station now.
[2023-11-02 21:00:59.110899] worker B put 1 part B on the station, 7 part Bs on the station now.
[2023-11-02 21:00:59.110904] worker B put 1 part B on the station, 8 part Bs on the station now.
[2023-11-02 21:00:59.110912] worker C took 4 part A from the station, 0 part As on the station now.
[2023-11-02 21:00:59.110918] worker C took 3 part B from the station, 5 part Bs on the station now.
[2023-11-02 21:00:59.110921] worker A put 2 part As on the station, 2 part As on the station now.
[2023-11-02 21:00:59.110923] worker C took 4 part A and 3 part B from the station, 5 empty on the station now.
[2023-11-02 21:00:59.110933] worker B put 1 part B on the station, 6 part Bs on the station now.
[2023-11-02 21:00:59.110940] worker B put 1 part B on the station, 7 part Bs on the station now.
[2023-11-02 21:00:59.110945] worker B put 1 part B on the station, 8 part Bs on the station now.
```

## 4-2 进程多类资源申请



## 4-3 医院门诊

