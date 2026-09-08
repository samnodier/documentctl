#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG_MODE
#include <stdio.h>
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif /* ifdef DEBUG_MODE */

#endif // DEBUG_H_
