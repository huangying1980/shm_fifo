#ifndef SHMFIFO_UTILS_H_
#define SHMFIFO_UTILS_H_

#include <linux/stddef.h>
#include <linux/swab.h>

#ifdef __cplusplus
extern "C" {
#endif
#define SHMFIFO_IS_POWER2(n) ((n) && !(((n) - 1) & (n)))

static inline uint64_t Combine64(uint64_t v)
{
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;

    return v;
}

static inline uint64_t Power2Align64(uint64_t v)
{
    v--;
    v = Combine64(v);
    return v + 1;
}

static inline uint32_t Combine32(uint32_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return x;
}

static inline uint32_t Power2Align32(uint32_t x)
{
    x--;
    x = Combine32(x);
    return x + 1;
}
#ifdef __cplusplus
}
#endif
#endif 
