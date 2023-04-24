#ifndef SHMFIFO_OBJ_POOL_H_
#define SHMFIFO_OBJ_POOL_H_

#include <unistd.h>

#include "shmfifo_ring.h"
#include "shmfifo_define.h"

#ifdef __cplusplus 
extern "C" {
#endif
#define SHMFIFO_OBJ_DATA(_fifo, _obj) ((_fifo)->start_addr + (_obj).offset)
#define SHMFIFO_OBJ_SIZE(_obj) ((_obj).size)

int ShmFifoObjPoolInit(struct ShmFifoRing *obj_pool, size_t msg_size, uint32_t msg_count);

static inline ssize_t
ShmFifoObjAlloc(struct ShmFifoRing *obj_pool, struct ShmFifoObj* const obj)
{
  return ShmFifoRingDequeueBulk(obj_pool, obj, 1, NULL);
}

static inline ssize_t
ShmFifoObjFree(struct ShmFifoRing *obj_pool, struct ShmFifoObj* const obj)
{
  SHMFIFO_OBJ_SIZE(*obj) = 0;
  return ShmFifoRingEnqueueBulk(obj_pool, obj, 1, NULL);
}

#ifdef __cplusplus 
}
#endif
#endif


