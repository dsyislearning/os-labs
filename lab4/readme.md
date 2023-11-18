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

### 问题描述

校医院口腔科每天向患者提供 $N=30$ 个挂号就诊名额。患者到达医院后，如果有号，则挂号，并在候诊室排队等待就医；如果号已满，则离开医院。

在诊疗室内，有 $M=3$ 位医生为患者提供治疗服务。如果候诊室有患者等待并且诊疗室内有医生处于“休息”态, 则从诊疗室挑选一位患者，安排一位医生为其治疗，医生转入“工作”状态；如果三位医生均处于“工作”态，候诊室内患者需等待。当无患者候诊时，医生转入“休息”状态。

要求:

1. 采用信号量机制,描述患者、医生的行为;
2. 设置医生忙闲状态向量 `DState[M]`,记录每位医生的“工作”、“休息”状态;
3. 设置患者就诊状态向量 `PState[N]`，记录挂号成功后的患者的“候诊”、“就医”状态

### 原理分析

本问题类似于睡眠理发师问题，既有多个进程互斥竞争使用资源，又有进程间同步。但与之不同之处在于：

1. 睡眠理发师问题中只有一位理发师，本问题中有多位医生为患者服务;
2. 引入两个作为共享数据结构的状态向量 `DState[M]`、`PState[N]`。多个医生对 `DState[M]` 的访问必须互斥，多个患者对 `PState[N]` 的访问也必须互斥，为此，需要引入两个互斥信号量；
3. 医院每天只放 $N=30$ 个号，最先挂号的前 $N$ 个患者可以获得号，并进入候诊室等待治疗。之后的患者由于拿不到当天号，没有必要等待，可以离开；而在睡眠理发师问题中，只要等候室有空位，顾客就可以进入等候室，等待理发服务。

### 代码实现

每个医生有一个单独的线程，每个病人也有一个单独的线程

```c
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#include "utils.h"

#define PATIENT_MAX 30
#define DOCTOR_MAX 3

#define WORKING 0
#define RESTING 1

#define EMPTY 0
#define WAITING 1
#define CURED 2

int DState[DOCTOR_MAX] = {RESTING};
int PState[PATIENT_MAX] = {EMPTY};
int registered = 0, cured = 0;

pthread_mutex_t D_mutex, P_mutex, r_mutex, c_mutex, i_mutex;

void * doctor(void * args)
{
    int id = *(int *)args;
    pthread_mutex_unlock(&i_mutex); // 保证线程创建顺序

    bool is_working = false; // 是否正在工作
    while (true) {
        pthread_mutex_lock(&D_mutex);
        if (DState[id] == RESTING) { // 休息状态
            pthread_mutex_lock(&P_mutex);
            int i;
            for (i = 0; i < PATIENT_MAX && PState[i] != WAITING; i++) // 找到一个等待的病人
                ;
            if (i < PATIENT_MAX) { // 找到了
                is_working = true;
                print_log("Doctor %d start to cure patient %d.", id, i);
                PState[i] = CURED; // 标记为已治愈
                DState[id] = WORKING; // 标记为工作状态
            }
            pthread_mutex_unlock(&P_mutex);
        } else {
            is_working = false; // 正在工作
            DState[id] = RESTING; // 标记为休息状态
            print_log("Doctor %d finished curing patient.", id); // 治愈一个病人
            pthread_mutex_lock(&c_mutex);
            cured++; // 治愈人数加一
            if (cured >= PATIENT_MAX) { // 如果所有人都治愈了
                pthread_mutex_unlock(&D_mutex);
                pthread_mutex_unlock(&c_mutex);
                break; // 退出循环
            }
            pthread_mutex_unlock(&c_mutex);
        }
        pthread_mutex_unlock(&D_mutex);
        if (is_working) {
            sleep(1); // 治愈一个病人需要一秒
        }
    }
    pthread_exit(NULL);
}

void * patient(void * args)
{
    int id = *(int *)args;
    pthread_mutex_unlock(&i_mutex); // 保证线程创建顺序

    pthread_mutex_lock(&P_mutex);
    int i;
    for (i = 0; i < PATIENT_MAX && PState[i] != EMPTY; i++) // 找到一个空位
        ;
    if (i < PATIENT_MAX) { // 找到了
        PState[i] = WAITING; // 标记为等待状态
        pthread_mutex_lock(&r_mutex);
        registered++;
        pthread_mutex_unlock(&r_mutex);
        print_log("Patient %d get register number %d.", id, i);
    } else {
        print_log("Patient %d leaved.", id);
    }
    pthread_mutex_unlock(&P_mutex);
    pthread_exit(NULL);
}

int main()
{
    pthread_mutex_init(&D_mutex, NULL);
    pthread_mutex_init(&P_mutex, NULL);
    pthread_mutex_init(&r_mutex, NULL);
    pthread_mutex_init(&c_mutex, NULL);
    pthread_mutex_init(&i_mutex, NULL);

    pthread_t Did[DOCTOR_MAX], Pid[PATIENT_MAX];
    
    // 创建三个医生线程
    for (int i = 0; i < DOCTOR_MAX; i++) {
        pthread_mutex_lock(&i_mutex);
        int id = i;
        if (pthread_create(&Did[i], NULL, &doctor, &id) != 0) {
            perror("Failed to create doctor thread");
            return 1;
        }
    }
    // 创建六十个病人线程
    for (int i = 0; i < PATIENT_MAX * 2; i++) {
        pthread_mutex_lock(&i_mutex);
        int id = i;
        if (pthread_create(&Pid[i], NULL, &patient, &id) != 0) {
            perror("Failed to create patient thread");
            return 1;
        }
    }

    // 等待所有线程结束
    for (int i = 0; i < DOCTOR_MAX; i++) {
        pthread_join(Did[i], NULL);
    }
    for (int i = 0; i < PATIENT_MAX * 2; i++) {
        pthread_join(Pid[i], NULL);
    }

    return 0;
}

```

### 测试结果

前三十个病人挂到号，后三十个病人离开

```
# dsy @ arch in ~/Code/bupt/os_labs/lab4 on git:main x [20:29:09] 
$ ./clinic              
[2023-11-18 20:29:09.898434] Doctor 1 finished curing patient.
[2023-11-18 20:29:09.898648] Doctor 2 finished curing patient.
[2023-11-18 20:29:09.899045] Patient 0 get register number 0.
[2023-11-18 20:29:09.899138] Doctor 0 start to cure patient 0.
[2023-11-18 20:29:09.899226] Patient 1 get register number 1.
[2023-11-18 20:29:09.899341] Doctor 2 start to cure patient 1.
[2023-11-18 20:29:09.899480] Patient 2 get register number 2.
[2023-11-18 20:29:09.899664] Doctor 1 start to cure patient 2.
[2023-11-18 20:29:09.899711] Patient 3 get register number 3.
[2023-11-18 20:29:09.899784] Patient 4 get register number 4.
[2023-11-18 20:29:09.899850] Patient 5 get register number 5.
[2023-11-18 20:29:09.899925] Patient 6 get register number 6.
[2023-11-18 20:29:09.900500] Patient 7 get register number 7.
[2023-11-18 20:29:09.900607] Patient 8 get register number 8.
[2023-11-18 20:29:09.900759] Patient 9 get register number 9.
[2023-11-18 20:29:09.900812] Patient 10 get register number 10.
[2023-11-18 20:29:09.900892] Patient 11 get register number 11.
[2023-11-18 20:29:09.900942] Patient 12 get register number 12.
[2023-11-18 20:29:09.901015] Patient 13 get register number 13.
[2023-11-18 20:29:09.901560] Patient 14 get register number 14.
[2023-11-18 20:29:09.901624] Patient 15 get register number 15.
[2023-11-18 20:29:09.901769] Patient 16 get register number 16.
[2023-11-18 20:29:09.901829] Patient 17 get register number 17.
[2023-11-18 20:29:09.901880] Patient 18 get register number 18.
[2023-11-18 20:29:09.901940] Patient 19 get register number 19.
[2023-11-18 20:29:09.901999] Patient 20 get register number 20.
[2023-11-18 20:29:09.902055] Patient 21 get register number 21.
[2023-11-18 20:29:09.902116] Patient 22 get register number 22.
[2023-11-18 20:29:09.902193] Patient 23 get register number 23.
[2023-11-18 20:29:09.902258] Patient 24 get register number 24.
[2023-11-18 20:29:09.902322] Patient 25 get register number 25.
[2023-11-18 20:29:09.902390] Patient 26 get register number 26.
[2023-11-18 20:29:09.902461] Patient 27 get register number 27.
[2023-11-18 20:29:09.902533] Patient 28 get register number 28.
[2023-11-18 20:29:09.902607] Patient 29 get register number 29.
[2023-11-18 20:29:09.902677] Patient 30 leaved.
[2023-11-18 20:29:09.902743] Patient 31 leaved.
[2023-11-18 20:29:09.902812] Patient 32 leaved.
[2023-11-18 20:29:09.902877] Patient 33 leaved.
[2023-11-18 20:29:09.902948] Patient 34 leaved.
[2023-11-18 20:29:09.903020] Patient 35 leaved.
[2023-11-18 20:29:09.903071] Patient 36 leaved.
[2023-11-18 20:29:09.903137] Patient 37 leaved.
[2023-11-18 20:29:09.903217] Patient 38 leaved.
[2023-11-18 20:29:09.903296] Patient 39 leaved.
[2023-11-18 20:29:09.903368] Patient 40 leaved.
[2023-11-18 20:29:09.903437] Patient 41 leaved.
[2023-11-18 20:29:09.903516] Patient 42 leaved.
[2023-11-18 20:29:09.903583] Patient 43 leaved.
[2023-11-18 20:29:09.903652] Patient 44 leaved.
[2023-11-18 20:29:09.903715] Patient 45 leaved.
[2023-11-18 20:29:09.903782] Patient 46 leaved.
[2023-11-18 20:29:09.903849] Patient 47 leaved.
[2023-11-18 20:29:09.903914] Patient 48 leaved.
[2023-11-18 20:29:09.903979] Patient 49 leaved.
[2023-11-18 20:29:09.904058] Patient 50 leaved.
[2023-11-18 20:29:09.904128] Patient 51 leaved.
[2023-11-18 20:29:09.904191] Patient 52 leaved.
[2023-11-18 20:29:09.904263] Patient 53 leaved.
[2023-11-18 20:29:09.904335] Patient 54 leaved.
[2023-11-18 20:29:09.904399] Patient 55 leaved.
[2023-11-18 20:29:09.904472] Patient 56 leaved.
[2023-11-18 20:29:09.904533] Patient 57 leaved.
[2023-11-18 20:29:09.904605] Patient 58 leaved.
[2023-11-18 20:29:09.904670] Patient 59 leaved.
[2023-11-18 20:29:10.901462] Doctor 1 finished curing patient.
[2023-11-18 20:29:10.901510] Doctor 1 start to cure patient 3.
[2023-11-18 20:29:10.901523] Doctor 2 finished curing patient.
[2023-11-18 20:29:10.901529] Doctor 2 start to cure patient 4.
[2023-11-18 20:29:10.901537] Doctor 0 finished curing patient.
[2023-11-18 20:29:10.901555] Doctor 0 start to cure patient 5.
[2023-11-18 20:29:11.901734] Doctor 1 finished curing patient.
[2023-11-18 20:29:11.901800] Doctor 1 start to cure patient 6.
[2023-11-18 20:29:11.901821] Doctor 2 finished curing patient.
[2023-11-18 20:29:11.901878] Doctor 2 start to cure patient 7.
[2023-11-18 20:29:11.901917] Doctor 0 finished curing patient.
[2023-11-18 20:29:11.901954] Doctor 0 start to cure patient 8.
[2023-11-18 20:29:12.902034] Doctor 1 finished curing patient.
[2023-11-18 20:29:12.902102] Doctor 1 start to cure patient 9.
[2023-11-18 20:29:12.902199] Doctor 2 finished curing patient.
[2023-11-18 20:29:12.902237] Doctor 2 start to cure patient 10.
[2023-11-18 20:29:12.902247] Doctor 0 finished curing patient.
[2023-11-18 20:29:12.902289] Doctor 0 start to cure patient 11.
[2023-11-18 20:29:13.902209] Doctor 1 finished curing patient.
[2023-11-18 20:29:13.902270] Doctor 1 start to cure patient 12.
[2023-11-18 20:29:13.902346] Doctor 2 finished curing patient.
[2023-11-18 20:29:13.902385] Doctor 2 start to cure patient 13.
[2023-11-18 20:29:13.902407] Doctor 0 finished curing patient.
[2023-11-18 20:29:13.902451] Doctor 0 start to cure patient 14.
[2023-11-18 20:29:14.902494] Doctor 1 finished curing patient.
[2023-11-18 20:29:14.902554] Doctor 1 start to cure patient 15.
[2023-11-18 20:29:14.902576] Doctor 2 finished curing patient.
[2023-11-18 20:29:14.902630] Doctor 2 start to cure patient 16.
[2023-11-18 20:29:14.902658] Doctor 0 finished curing patient.
[2023-11-18 20:29:14.902692] Doctor 0 start to cure patient 17.
[2023-11-18 20:29:15.902783] Doctor 1 finished curing patient.
[2023-11-18 20:29:15.902863] Doctor 1 start to cure patient 18.
[2023-11-18 20:29:15.902886] Doctor 0 finished curing patient.
[2023-11-18 20:29:15.902938] Doctor 0 start to cure patient 19.
[2023-11-18 20:29:15.902955] Doctor 2 finished curing patient.
[2023-11-18 20:29:15.902991] Doctor 2 start to cure patient 20.
[2023-11-18 20:29:16.903103] Doctor 1 finished curing patient.
[2023-11-18 20:29:16.903181] Doctor 1 start to cure patient 21.
[2023-11-18 20:29:16.903193] Doctor 2 finished curing patient.
[2023-11-18 20:29:16.903240] Doctor 2 start to cure patient 22.
[2023-11-18 20:29:16.903252] Doctor 0 finished curing patient.
[2023-11-18 20:29:16.903304] Doctor 0 start to cure patient 23.
[2023-11-18 20:29:17.903417] Doctor 1 finished curing patient.
[2023-11-18 20:29:17.903478] Doctor 1 start to cure patient 24.
[2023-11-18 20:29:17.903505] Doctor 0 finished curing patient.
[2023-11-18 20:29:17.903553] Doctor 0 start to cure patient 25.
[2023-11-18 20:29:17.903569] Doctor 2 finished curing patient.
[2023-11-18 20:29:17.903603] Doctor 2 start to cure patient 26.
[2023-11-18 20:29:18.903704] Doctor 1 finished curing patient.
[2023-11-18 20:29:18.903770] Doctor 2 finished curing patient.
[2023-11-18 20:29:18.903813] Doctor 2 start to cure patient 27.
[2023-11-18 20:29:18.903855] Doctor 0 finished curing patient.
[2023-11-18 20:29:18.903896] Doctor 0 start to cure patient 28.
[2023-11-18 20:29:18.903927] Doctor 1 start to cure patient 29.
[2023-11-18 20:29:19.904045] Doctor 2 finished curing patient.
[2023-11-18 20:29:19.904145] Doctor 1 finished curing patient.
[2023-11-18 20:29:19.904204] Doctor 0 finished curing patient.
```

