#include "shmfifo_obj_pool.h"

#include <stdint.h>
#include <stdlib.h>

#include "shmfifo_error.h"

int ShmFifoObjPoolInit(struct ShmFifoRing *obj_pool, size_t msg_size, uint32_t msg_count)
{
  uint32_t        i;
  unsigned int    n;
  struct ShmFifoObj *obj_list;
  
  ShmFifoRingInit(obj_pool, msg_count, SHMFIFO_RING_SP_ENQ | SHMFIFO_RING_SC_DEQ);
  obj_list = (struct ShmFifoObj *)malloc(sizeof(struct ShmFifoObj) * msg_count);
  for (i = 0; i < msg_count; ++i) {
    obj_list[i].offset = i * msg_size;
    obj_list[i].size = 0;
  }
  n = ShmFifoRingEnqueueBulk(obj_pool, obj_list, msg_count, NULL);
  free(obj_list);
  if (n != msg_count) {
    return -SHMFIFO_ERR_OBJ_POOL_INIT_ENQUEUE;
  }
  return SHMFIFO_ERR_NO;
}
