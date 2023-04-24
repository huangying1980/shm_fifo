#include "shmfifo_ring.h"

void ShmFifoRingInit(struct ShmFifoRing *ring, uint32_t count, int flags)
{
  ring->size = count;
  ring->mask = count - 1;
  ring->capacity = count;
  ring->prod.head = ring->prod.tail = 0;
  ring->cons.head = ring->cons.tail = 0;
  if (flags & SHMFIFO_RING_SP_ENQ) {
    ring->prod.single = 1;
  } else {
    ring->prod.single = 0;
  }
  if (flags & SHMFIFO_RING_SC_DEQ) {
    ring->cons.single = 1;
  } else {
    ring->cons.single = 0;
  }
}
