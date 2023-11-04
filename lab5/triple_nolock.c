#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "structure.h"

void * apple_a(void * arg)
{
    struct apple * app = (struct apple *) arg;

    for (int i = 0; i < APPLE_MAX_VALUE; i++) {
        app->a += i;
    }

    return NULL;
}

void * apple_b(void * arg)
{
    struct apple * app = (struct apple *) arg;

    for (int i = 0; i < APPLE_MAX_VALUE; i++) {
        app->b += i;
    }

    return NULL;
}

void * orange(void * arg)
{
    struct orange * oran = (struct orange *) arg;

    unsigned long long sum;
    for (int i = 0; i < ORANGE_MAX_VALUE; i++) {
        sum += oran->a[i] + oran->b[i];
    }

    return NULL;
}

int main(int argc, char * argv[])
{
    struct apple app;
    struct orange oran;

    clock_t start, end;
    double cpu_time_used;

    start = clock();

    pthread_t pid[3];
    pthread_create(&pid[0], NULL, apple_a, &app);
    pthread_create(&pid[1], NULL, apple_b, &app);
    pthread_create(&pid[2], NULL, orange, &oran);

    pthread_join(pid[0], NULL);
    pthread_join(pid[1], NULL);
    pthread_join(pid[2], NULL);

    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Time elapsed: %f seconds.\n", cpu_time_used);

    return 0;
}
