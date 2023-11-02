#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#define print_log(format, ...) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    time_t nowtime = tv.tv_sec; \
    struct tm *nowtm = localtime(&nowtime); \
    char tmbuf[64], buf[64]; \
    strftime(tmbuf, sizeof tmbuf, "[%Y-%m-%d %H:%M:%S", nowtm); \
    snprintf(buf, sizeof buf, "%s.%06ld]", tmbuf, tv.tv_usec); \
    printf("%s ", buf); \
    printf(format, ##__VA_ARGS__); \
    printf("\n"); \
} while(0)

#endif // UTILS_H
