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
