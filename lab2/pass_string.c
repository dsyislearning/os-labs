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
