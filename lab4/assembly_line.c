#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>

#include "utils.h"

#define N1 4
#define N2 3
#define M1 2
#define M2 1
#define N 12

int countA = 0, countB = 0, count_empty = N;
pthread_mutex_t mutex_A, mutex_B, mutex_empty;

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

int main()
{
    pthread_mutex_init(&mutex_A, NULL);
    pthread_mutex_init(&mutex_B, NULL);
    pthread_mutex_init(&mutex_empty, NULL);

    pthread_t thread_A, thread_B, thread_C;
    pthread_create(&thread_A, NULL, routine_A, NULL);
    pthread_create(&thread_B, NULL, routine_B, NULL);
    pthread_create(&thread_C, NULL, routine_C, NULL);

    pthread_join(thread_A, NULL);
    pthread_join(thread_B, NULL);
    pthread_join(thread_C, NULL);

    return 0;
}
