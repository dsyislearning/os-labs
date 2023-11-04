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
