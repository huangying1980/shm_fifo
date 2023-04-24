#ifndef SHMFIFO_LOG_H_
#define SHMFIFO_LOG_H_

#ifdef __cplusplus 
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>


#if defined SHMFIFO_DEBUG

#define SHMFIFO_DEBUG_OUT(format, args...) \
fprintf(stderr, "[SHMFIFO_DEBUG] ");  \
fprintf(stderr, format, ##args); \
fprintf(stderr, "\n")

#elif defined SHMFIFO_DEBUG_VERBOSE

#define SHMFIFO_DEBUG_OUT(format, args...) \
    fprintf(stderr, "[%s:%d %s SHMFIFO_DEBUG] ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, format, ##args);\
    fprintf(stderr, "\n")
#else

#define SHMFIFO_DEBUG_OUT(format, args...) do{}while(0)

#endif

#ifdef SHMFIFO_ERROR_VERBOSE

#define SHMFIFO_OOPS_OUT(format, args...) \
    fprintf(stderr, "[%s:%d %s SHMFIFO_OOPS] ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, format, ##args);\
    fprintf(stderr, "\n"); \
    abort()

#define SHMFIFO_ERR_OUT(format, args...) \
    fprintf(stderr, "[%s:%d %s SHMFIFO_ERR] ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, format, ##args);\
    fprintf(stderr, "\n")

#define SHMFIFO_WARN_OUT(format, args...) \
    fprintf(stderr, "[%s:%d %s SHMFIFO_WARN] ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, format, ##args);\
    fprintf(stderr, "\n")

#else

#define SHMFIFO_ERR_OUT(format, args...) \
fprintf(stderr, "[SHMFIFO_ERR] ");  \
fprintf(stderr, format, ##args); \
fprintf(stderr, "\n")

#define SHMFIFO_WARN_OUT(format, args...) \
fprintf(stderr, "[SHMFIFO_WARN] ");  \
fprintf(stderr, format, ##args); \
fprintf(stderr, "\n")

#define SHMFIFO_OOPS_OUT(format, args...) \
fprintf(stderr, "[SHMFIFO_OOPS] "); \
fprintf(stderr, format, ##args); \
fprintf(stderr, "\n");\
abort()

#endif

#ifdef __cplusplus 
}
#endif

#endif
