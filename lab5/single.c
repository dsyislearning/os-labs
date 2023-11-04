#include <stdio.h>
#include <time.h>

#include "structure.h"

int main(int argc, char * argv[])
{
    struct apple app;
    struct orange oran;

    clock_t start, end;
    double cpu_time_used;

    start = clock();

    for (int i = 0; i < APPLE_MAX_VALUE; i++) {
        app.a += i;
        app.b += i;
    }

    unsigned long long sum;
    for (int i = 0; i < ORANGE_MAX_VALUE; i++) {
        sum += oran.a[i] + oran.b[i];
    }

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Time elapsed: %f seconds.\n", cpu_time_used);

    return 0;
}
