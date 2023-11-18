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
