#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>

#define ERROR_EXIT(msg, ...)                        \
    do {                                            \
        fprintf(stderr, "[ERROR:%s:%d]: " msg,      \
                __FILE__, __LINE__, ##__VA_ARGS__); \
        exit(2);                                    \
    } while (0)

#define __maybe_unused __attribute__((unused))

#endif /* __UTILS_H__ */
