#ifndef SHMFIFO_RING_H_
#define SHMFIFO_RING_H_
#include <emmintrin.h>
#include <stdint.h>

#include "shmfifo_define.h"
#include "shmfifo_error.h"
#include "shmfifo_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHMFIFO_RING_SP_ENQ 0x0001
#define SHMFIFO_RING_SC_DEQ 0x0002


struct ShmFifoHeadTail {
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t          single;
};
struct ShmFifoRing{
    uint32_t            size;
    uint32_t            mask;
    uint32_t            capacity;
    char                pad0[0] SHMFIFO_CACHELINE_ALIGN;
    struct ShmFifoHeadTail prod SHMFIFO_CACHELINE_ALIGN;
    char                pad1[0] SHMFIFO_CACHELINE_ALIGN;
    struct ShmFifoHeadTail cons SHMFIFO_CACHELINE_ALIGN;
} SHMFIFO_CACHELINE_ALIGN;

void ShmFifoRingInit(struct ShmFifoRing *ring, uint32_t count, int flags);

#define SHMFIFO_ENQUEUE_ADDR(r, ring_start, prod_head, obj_table, n) \
do { \
	unsigned int i; \
	const uint32_t size = (r)->size; \
	uint32_t idx = prod_head & (r)->mask; \
	struct ShmFifoObj *_ring = (struct ShmFifoObj *)ring_start; \
	if (shmfifo_likely(idx + n < size)) { \
		for (i = 0; i < (n & ((~(unsigned)0x3))); i += 4, idx += 4) { \
			_ring[idx] = obj_table[i]; \
			_ring[idx + 1] = obj_table[i + 1]; \
			_ring[idx + 2] = obj_table[i + 2]; \
			_ring[idx + 3] = obj_table[i + 3]; \
		} \
		switch (n & 0x3) { \
		case 3: \
			_ring[idx++] = obj_table[i++];  \
		case 2: \
			_ring[idx++] = obj_table[i++];  \
		case 1: \
			_ring[idx++] = obj_table[i++]; \
		} \
	} else { \
		for (i = 0; idx < size; i++, idx++)\
			_ring[idx] = obj_table[i]; \
		for (idx = 0; i < n; i++, idx++) \
			_ring[idx] = obj_table[i]; \
	} \
} while (0)

#define SHMFIFO_DEQUEUE_ADDR(r, ring_start, cons_head, obj_table, n) \
do { \
	unsigned int i; \
	uint32_t idx = cons_head & (r)->mask; \
	const uint32_t size = (r)->size; \
	struct ShmFifoObj *_ring = (struct ShmFifoObj *)ring_start; \
	if (shmfifo_likely(idx + n < size)) { \
		for (i = 0; i < (n & (~(unsigned)0x3)); i += 4, idx += 4) {\
			obj_table[i] = _ring[idx]; \
			obj_table[i + 1] = _ring[idx + 1]; \
			obj_table[i + 2] = _ring[idx + 2]; \
			obj_table[i + 3] = _ring[idx + 3]; \
		} \
		switch (n & 0x3) { \
		case 3: \
			obj_table[i++] = _ring[idx++];  \
		case 2: \
			obj_table[i++] = _ring[idx++];  \
		case 1: \
			obj_table[i++] = _ring[idx++]; \
		} \
	} else { \
		for (i = 0; idx < size; i++, idx++) \
			obj_table[i] = _ring[idx]; \
		for (idx = 0; i < n; i++, idx++) \
			obj_table[i] = _ring[idx]; \
	} \
} while (0)

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >=2)
#define ShmFifoRingCmpset32(val, old, set) \
        __sync_bool_compare_and_swap((val), (old), (set))
#else
static inline int
ShmFifoRingCmpset32(volatile uint32_t *dst, uint32_t exp, uint32_t src)
{
    uint8_t res;
    asm volatile(
        "lock;"
        "cmpxchgl %[src], %[dst];"
        "sete %[res];"
        : [res] "=a" (res), 
        [dst] "=m" (*dst)
        : [src] "r" (src), 
        "a" (exp),
        "m" (*dst)
        : "memory");   
    return res;
}
#endif

static inline unsigned int
ShmFifoRingMoveProdHead(struct ShmFifoRing *ring,
    unsigned int is_sp, unsigned int n,
    uint32_t *old_head, uint32_t *new_head, uint32_t *free_entries)
{
  const uint32_t  capacity = ring->capacity;
  unsigned int    max = n;
  int             ret = 0;

  do {
    n = max;
    *old_head = ring->prod.head;
    SHMFIFO_RMB();
    *free_entries = capacity + ring->cons.tail - *old_head; //???
    if (shmfifo_unlikely(n > *free_entries)) {
      n = *free_entries;
    }
    *new_head = *old_head + n;
    if (is_sp) {
      ring->prod.head = *new_head;
      ret = 1;
    } else {
      ret = ShmFifoRingCmpset32(&ring->prod.head, *old_head, *new_head);
    }

  } while (shmfifo_unlikely(!ret));

  return n;
}

static inline unsigned int
ShmFifoRingMoveConsHead(struct ShmFifoRing *ring,
    unsigned int is_sc, unsigned int n,
    uint32_t *old_head, uint32_t *new_head, uint32_t *entries)
{
  unsigned int max = n;
  int          ret = 0;

  do {
    n = max;
    *old_head = ring->cons.head;
    SHMFIFO_RMB();
    *entries = ring->prod.tail - *old_head;
    if (shmfifo_unlikely(n > *entries)) {
      n = *entries;
    }
    *new_head = *old_head + n;
    if (is_sc) {
      ring->cons.head = *new_head;
      ret = 1;
    } else {
      ret = ShmFifoRingCmpset32(&ring->cons.head, *old_head, *new_head);
    }
  } while (shmfifo_unlikely(!ret));

  return n;
}

static inline void
ShmFifoRingUpdateTail(struct ShmFifoHeadTail *ht,
    uint32_t old_val, uint32_t new_val,
    uint32_t single, uint32_t enq)
{
  if (enq) {
    SHMFIFO_RMB();
  } else {
    SHMFIFO_WMB();
  }

  if (!single) {
    while (shmfifo_unlikely(ht->tail != old_val)) {
      SHMFIFO_PAUSE();
    }
  }
  ht->tail = new_val;
}

static inline unsigned int
ShmFifoRingDoEnqueue(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n,
    unsigned int is_sp, unsigned int *free_space)
{
  uint32_t prod_head;
  uint32_t prod_next;
  uint32_t free_entries;

  n = ShmFifoRingMoveProdHead(ring, is_sp, n,
      &prod_head, &prod_next, &free_entries);
  if (!n) {
    goto out;
  }
  SHMFIFO_ENQUEUE_ADDR(ring, &ring[1], prod_head, obj_list, n);
  ShmFifoRingUpdateTail(&ring->prod, prod_head, prod_next, is_sp, 1);
out:
  if (free_space) {
    *free_space = free_entries - n;
  }
  return n; 
}

static inline unsigned int
ShmFifoRingDoDequeue(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n,
    unsigned int is_sc, unsigned int *available)
{
  uint32_t cons_head;
  uint32_t cons_next;
  uint32_t entries;

  n = ShmFifoRingMoveConsHead(ring, is_sc, n,
      &cons_head, &cons_next, &entries);
  if (!n) {
    goto out;
  }
  SHMFIFO_DEQUEUE_ADDR(ring, &ring[1], cons_head, obj_list, n);
  ShmFifoRingUpdateTail(&ring->cons, cons_head, cons_next, is_sc, 0);
out:
  if (available) {
    *available = entries - n;
  }

  return n;
}

static inline unsigned int
ShmFifoRingMultiProdEnqueueBulk(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n,
    unsigned int *free_space)
{
  return ShmFifoRingDoEnqueue(ring, obj_list, n, 0, free_space);
}

static inline unsigned int
ShmFifoRingSingleProdEnqueueBulk(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n,
    unsigned int *free_space)
{
  return ShmFifoRingDoEnqueue(ring, obj_list, n, 1, free_space);
}

static inline unsigned int
ShmFifoRingEnqueueBulk(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n,
    unsigned int *free_space)
{
  return ShmFifoRingDoEnqueue(ring, obj_list, n,
      ring->prod.single, free_space);
}

static inline unsigned int
ShmFifoRingMultiConsDequeueBulk(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n, unsigned int *available)
{
  return ShmFifoRingDoDequeue(ring, obj_list, n, 0, available);
}

static inline unsigned int
ShmFifoRinigSingleConsDequeueBulk(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n, unsigned int *available)
{
  return ShmFifoRingDoDequeue(ring, obj_list, n, 1, available);
}

static inline unsigned int
ShmFifoRingDequeueBulk(struct ShmFifoRing *ring,
    struct ShmFifoObj *obj_list, unsigned int n,
    unsigned int *available)
{
  return ShmFifoRingDoDequeue(ring, obj_list, n,
      ring->cons.single, available);
}

static inline unsigned int
ShmFifoRingCount(const struct ShmFifoRing *ring)
{
  uint32_t prod_tail = ring->prod.tail;
  uint32_t cons_tail = ring->cons.tail;
  uint32_t count = (prod_tail - cons_tail) & ring->mask;
  return (count > ring->capacity) ? ring->capacity : count;
}

static inline unsigned int
ShmFifoRingFreeCount(struct ShmFifoRing *ring)
{
  return ring->capacity - ShmFifoRingCount(ring);
}

static inline unsigned int
ShmFifoRingFull(struct ShmFifoRing *ring)
{
  return !ShmFifoRingFreeCount(ring);
}

static inline unsigned int
ShmFifoRingEmpty(const struct ShmFifoRing *ring)
{
  return !ShmFifoRingCount(ring);
}

static inline unsigned int
ShmFifoRingGetSize(const struct ShmFifoRing *r)
{   
  return r->size;
}

static inline unsigned int
ShmFifoRingGetCapacity(const struct ShmFifoRing *r)
{   
  return r->capacity;
}

static inline unsigned int
ShmFifoRingHead(const struct ShmFifoRing *ring, struct ShmFifoObj *obj)
{
  uint32_t  head;

  if (shmfifo_unlikely(!ShmFifoRingEmpty(ring))) {
    return -SHMFIFO_ERR_EMPTY;
  }
  do {
    head = ring->cons.head;
  } while (!ShmFifoRingCmpset32(&head, ring->cons.head, ring->cons.head));

  SHMFIFO_DEQUEUE_ADDR(ring, &ring[1], head, obj, 1);

  return SHMFIFO_ERR_NO;
}
#ifdef __cplusplus
} 
#endif
#endif
