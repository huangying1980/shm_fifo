#ifndef SHMFIFO_DEFINE_H_
#define SHMFIFO_DEFINE_H_
#include <stdint.h>
#include <emmintrin.h>
#include <linux/limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHMFIFO_TRUE 1
#define SHMFIFO_FALSE 0

#define SHMFIFO_INVALID_FD (-1)

#ifndef typeof
#define typeof __typeof__
#endif

#define SHMFIFO_OFFSETOF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define SHMFIFO_CONTAINER_OF(ptr, type, member) ({			\
	(type *)( (char *)ptr - SHMFIFO_OFFSETOF(type, member) );})

#define SHMFIFO_SIZE_ALIGN(_s, _m) (((_s) + (_m) - 1) & ~((_m) - 1))
#define SHMFIFO_CACHE_LINE (64)
#define SHMFIFO_CACHELINE_ALIGN __attribute__((aligned(SHMFIFO_CACHE_LINE)))

#ifndef SHMFIFO_PAGE_SIZE
#define SHMFIFO_PAGE_SIZE (4096UL)
#endif

#ifdef SHMFIFO_FENCE
#define SHMFIFO_RMB()   _mm_lfence()
#define SHMFIFO_WMB()   _mm_sfence()
#define SHMFIFO_MB()    _mm_mfence()
#define SHMFIFO_PAUSE() _mm_pause()
#else
#define SHMFIFO_RMB() 
#define SHMFIFO_WMB()
#define SHMFIFO_MB()
#define SHMFIFO_PAUSE()
#endif

#define shmfifo_likely(x) __builtin_expect(!!(x), 1)
#define shmfifo_unlikely(x)  __builtin_expect(!!(x), 0)

#define SHMFIFO_PATH_MAX   PATH_MAX

#define SHMFIFO_MIN(x_, y_) ((y_) ^ (((x_) ^ (y_)) & -((x_) < (y_))))
struct ShmFifoObj
{
  size_t offset;
  size_t size;
};

#ifdef __cplusplus
}
#endif

#endif
