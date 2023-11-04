#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "structure.h"

void * apple(void * arg)
{
    struct apple * app = (struct apple *) arg;

    for (int i = 0; i < APPLE_MAX_VALUE; i++) {
        app->a += i;
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

    pthread_t pid[2];
    pthread_create(&pid[0], NULL, apple, &app);
    pthread_create(&pid[1], NULL, orange, &oran);

    pthread_join(pid[0], NULL);
    pthread_join(pid[1], NULL);

    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Time elapsed: %f seconds.\n", cpu_time_used);

    return 0;
}
