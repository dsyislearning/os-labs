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
    pthread_mutex_unlock(&i_mutex);

    bool is_working = false;
    while (true) {
        pthread_mutex_lock(&D_mutex);
        if (DState[id] == RESTING) {
            pthread_mutex_lock(&P_mutex);
            int i;
            for (i = 0; i < PATIENT_MAX && PState[i] != WAITING; i++)
                ;
            if (i < PATIENT_MAX) {
                is_working = true;
                print_log("Doctor %d start to cure patient %d.", id, i);
                PState[i] = CURED;
                DState[id] = WORKING;
            }
            pthread_mutex_unlock(&P_mutex);
        } else {
            is_working = false;
            DState[id] = RESTING;
            print_log("Doctor %d finished curing patient.", id);
            pthread_mutex_lock(&c_mutex);
            cured++;
            if (cured >= PATIENT_MAX) {
                pthread_mutex_unlock(&D_mutex);
                pthread_mutex_unlock(&c_mutex);
                break;
            }
            pthread_mutex_unlock(&c_mutex);
        }
        pthread_mutex_unlock(&D_mutex);
        if (is_working) {
            sleep(1);
        }
    }
    pthread_exit(NULL);
}

void * patient(void * args)
{
    int id = *(int *)args;
    pthread_mutex_unlock(&i_mutex);

    pthread_mutex_lock(&P_mutex);
    int i;
    for (i = 0; i < PATIENT_MAX && PState[i] != EMPTY; i++)
        ;
    if (i < PATIENT_MAX) {
        PState[i] = WAITING;
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
    
    for (int i = 0; i < DOCTOR_MAX; i++) {
        pthread_mutex_lock(&i_mutex);
        int id = i;
        if (pthread_create(&Did[i], NULL, &doctor, &id) != 0) {
            perror("Failed to create doctor thread");
            return 1;
        }
    }
    for (int i = 0; i < PATIENT_MAX * 2; i++) {
        pthread_mutex_lock(&i_mutex);
        int id = i;
        if (pthread_create(&Pid[i], NULL, &patient, &id) != 0) {
            perror("Failed to create patient thread");
            return 1;
        }
    }

    for (int i = 0; i < DOCTOR_MAX; i++) {
        pthread_join(Did[i], NULL);
    }
    for (int i = 0; i < PATIENT_MAX * 2; i++) {
        pthread_join(Pid[i], NULL);
    }

    return 0;
}
