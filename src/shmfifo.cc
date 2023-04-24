#include "shmfifo.h"

#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "shmfifo_ring.h"
#include "shmfifo_obj_pool.h"
#include "shmfifo_log.h"
#include "shmfifo_error.h"
#include "shmfifo_define.h"
#include "shmfifo_utils.h"

#ifndef SHMFIFO_MAGIC
#define SHMFIFO_MAGIC 0x4649464F //SHMFIFO
#endif

#ifndef SHMFIFO_MAJOR
#define SHMFIFO_MAJOR (1U)
#endif

#ifndef SHMFIFO_MINOR
#define SHMFIFO_MINOR  (0U)
#endif

#ifndef SHMFIFO_VERSION
#define SHMFIFO_VERSION ((SHMFIFO_MAJOR << 16) | SHMFIFO_MINOR)
#endif

#ifdef SHMFIFO_FAST_MEMCPY
#include "shmfifo_memcpy.h"
#define shmfifo_memcpy(_dst, _src, _size) fifo_fast_memcpy(_dst, _src, _size)
#else
#define shmfifo_memcpy(_dst, _src, _size) memcpy(_dst, _src, _size);
#endif

struct ShmFifo {
  uint32_t          magic;
  uint32_t          version;
  int               fd;
  char             *start_addr;
  size_t            total_size;
  size_t            list_size;
  size_t            msg_size;
  size_t            msg_count;
  time_t            create_time;
  pid_t             creator;
  struct ShmFifoRing  *list; 
  struct ShmFifoRing  *obj_pool; 
} SHMFIFO_CACHELINE_ALIGN;

static int ShmFifoFormat(int fd, size_t size);
static int ShmFifoFileReady(int fd, size_t size);
static int ShmFifoLoadVersion(int fd, uint32_t *magic, uint32_t *version);
static void ShmFifoReset(struct ShmFifo *fifo, size_t total_size, size_t list_size,
  size_t msg_size, size_t msg_count, int fd);

//#pragma GCC push_options
//#pragma GCC optimize("O0")
struct ShmFifo* ShmFifoOpen(const char *path, size_t msg_size, size_t msg_count)
{
  struct ShmFifo   *fifo = NULL;
  size_t         list_size;
  size_t         total_size;
  size_t         i;
  int            fd;
  int            ready;

  msg_size = SHMFIFO_SIZE_ALIGN(msg_size, 1024);
  msg_count = Power2Align32(msg_count + 1);
  list_size = sizeof(struct ShmFifoObj) * msg_count + sizeof(struct ShmFifoRing);
  list_size = SHMFIFO_SIZE_ALIGN(list_size, SHMFIFO_CACHE_LINE);
  total_size = msg_size * msg_count;
  total_size += sizeof(struct ShmFifo) + (list_size << 1);
  total_size = SHMFIFO_SIZE_ALIGN(total_size, SHMFIFO_PAGE_SIZE);

  fd = open(path, O_CREAT | O_RDWR, 0666);
  if (fd < 0) {
    SHMFIFO_ERR_OUT("ShmFifoOpen failed, open error %s, err %d", path, errno);
    return NULL;
  }
  ready = ShmFifoFileReady(fd, total_size); 
  if (ready == SHMFIFO_TRUE) {
    goto SHMFIFO_DO_MAP;
  }
  if (ShmFifoFormat(fd, total_size) != SHMFIFO_ERR_NO) {
    close(fd);
    goto SHMFIFO_DO_EXIT;
  }

SHMFIFO_DO_MAP:
  fifo = (struct ShmFifo *)mmap(NULL, total_size, PROT_READ | PROT_WRITE,
    MAP_SHARED, fd, 0); 
  if (!fifo || fifo == (void *)MAP_FAILED) {
    SHMFIFO_ERR_OUT("ShmFifoOpen failed, mmap error %s, err %d", path, errno);
    close(fd);
    goto SHMFIFO_DO_EXIT;
  }

  if (madvise(fifo, total_size, MADV_SEQUENTIAL) < 0) {
    SHMFIFO_ERR_OUT("ShmFifoOpen failed, madvise failed, err %d", errno);
    munmap(fifo, total_size);
    close(fd);
    goto SHMFIFO_DO_EXIT;
  }

  if (mlock(fifo, total_size) < 0) {
    SHMFIFO_ERR_OUT("ShmFifoOpen failed, mlock failed, err %d", errno);
    munmap(fifo, total_size);
    close(fd);
    goto SHMFIFO_DO_EXIT;
  }


  fifo->list = (struct ShmFifoRing *)&fifo[1];
  fifo->obj_pool = (struct ShmFifoRing *)((char *)fifo->list + list_size);
  fifo->start_addr = (char *)fifo->obj_pool + list_size;

  if (ready != SHMFIFO_TRUE) {
    ShmFifoReset(fifo, total_size, list_size, msg_size, msg_count, fd);
    ShmFifoObjPoolInit(fifo->obj_pool, msg_size, msg_count);
    ShmFifoRingInit(fifo->list, msg_count, SHMFIFO_RING_SP_ENQ | SHMFIFO_RING_SC_DEQ);
  } else if (fifo->msg_size != msg_size || fifo->msg_count != msg_count) {
    SHMFIFO_ERR_OUT("ShmFifoOpen capacity error, %lu * %lu != %lu * %lu",
      msg_size, msg_count, fifo->msg_size, fifo->msg_count);
    ShmFifoClose(fifo);
    goto SHMFIFO_DO_EXIT;
  }
  for (i = 0; i < total_size; i += SHMFIFO_PAGE_SIZE) {
    (void)(((char *)fifo)[i]);
  }  

SHMFIFO_DO_EXIT:
  return fifo;  
}
//#pragma GCC pop_options

void ShmFifoClose(struct ShmFifo *fifo)
{
  int fd = fifo->fd;
  munlock(fifo, fifo->total_size);  
  munmap(fifo, fifo->total_size);
  close(fd);
}

ssize_t ShmFifoPush(struct ShmFifo *fifo, const char *buf, const size_t buf_size)
{
  size_t            size;    
  struct ShmFifoObj    obj = {0, 0};

  if (shmfifo_unlikely(ShmFifoRingFull(fifo->list))) {
    SHMFIFO_DEBUG_OUT("ShmFifo full");
    return -SHMFIFO_ERR_FULL;
  }
  if (shmfifo_unlikely(!ShmFifoObjAlloc(fifo->obj_pool, &obj))) {
    SHMFIFO_ERR_OUT("ShmFifoPush failed, ShmFifoObjAlloc error");
    return -SHMFIFO_ERR_PUSH_OBJ_ALLOC;
  }
  size = SHMFIFO_MIN(fifo->msg_size, buf_size);
  shmfifo_memcpy(SHMFIFO_OBJ_DATA(fifo, obj), buf, size);
  SHMFIFO_OBJ_SIZE(obj) = size;
  if (shmfifo_unlikely(!ShmFifoRingEnqueueBulk(fifo->list, &obj, 1, NULL))) {
    SHMFIFO_ERR_OUT("ShmFifoPush failed, Enqueue error");
    ShmFifoObjFree(fifo->obj_pool, &obj);
    return -SHMFIFO_ERR_PUSH_OBJ_ENQUEUE;
  }
  return (ssize_t)size;
}

int ShmFifoPop(struct ShmFifo *fifo)
{
  struct ShmFifoObj obj;

  if (shmfifo_unlikely(!ShmFifoRingDequeueBulk(fifo->list, &obj, 1, NULL))) {
    SHMFIFO_ERR_OUT("ShmFifoPop failed, fifo empty");
    return -SHMFIFO_ERR_EMPTY;
  }

  if (shmfifo_unlikely(!ShmFifoObjFree(fifo->obj_pool, &obj))) {
    SHMFIFO_ERR_OUT("ShmFifoPop failed, ShmFifoObjFree error");
    return -SHMFIFO_ERR_POP_OBJ_FREE;
  }

  return SHMFIFO_ERR_NO;
}

ssize_t ShmFifoPopData(struct ShmFifo *fifo, char* const buf, const size_t buf_size)
{
  struct ShmFifoObj obj;
  
  if (shmfifo_unlikely(!ShmFifoRingDequeueBulk(fifo->list, &obj, 1, NULL))) {
    SHMFIFO_ERR_OUT("ShmFifoPop failed, fifo empty");
    return -SHMFIFO_ERR_EMPTY;
  }
  if (shmfifo_unlikely(buf_size < SHMFIFO_OBJ_SIZE(obj))) {
    SHMFIFO_ERR_OUT("ShmFifoPop failed, obj size %lu, buf size %lu error",
      SHMFIFO_OBJ_SIZE(obj), buf_size);
    return -SHMFIFO_ERR_POP_DATA_BUF_SIZE;
  }
  shmfifo_memcpy(buf, SHMFIFO_OBJ_DATA(fifo, obj), SHMFIFO_OBJ_SIZE(obj));  
  
  if (shmfifo_unlikely(!ShmFifoObjFree(fifo->obj_pool, &obj))) {
    SHMFIFO_ERR_OUT("ShmFifoPop failed, ShmFifoObjFree error");
    return -SHMFIFO_ERR_POP_DATA_OBJ_FREE;
  }
  
  return (ssize_t)obj.size;
}

void *ShmFifoTop(struct ShmFifo *fifo, size_t *size)
{
  struct ShmFifoObj obj = {0, 0};
  
  if (shmfifo_unlikely(!ShmFifoRingHead(fifo->list, &obj))) {
    SHMFIFO_DEBUG_OUT("ShmFifoTop failed, fifo empty");
    return NULL;
  }
  *size = SHMFIFO_OBJ_SIZE(obj);
  return SHMFIFO_OBJ_DATA(fifo, obj);
}

static void ShmFifoReset(struct ShmFifo *fifo, size_t total_size, size_t list_size, size_t msg_size,
  size_t msg_count, int fd)
{
  fifo->magic = SHMFIFO_MAGIC;
  fifo->version = SHMFIFO_VERSION;
  fifo->total_size = total_size;
  fifo->list_size = list_size;
  fifo->msg_size = msg_size;
  fifo->msg_count = msg_count;
  fifo->create_time = time(NULL);
  fifo->creator = getpid();
  fifo->fd = fd;
}

static int ShmFifoLoadVersion(int fd, uint32_t *magic, uint32_t *version)
{
  ssize_t ret = SHMFIFO_ERR_NO;
  struct ShmFifoMeta {
    uint32_t  magic;
    uint32_t  version;
  }meta = {0, 0};
  
  ret = pread(fd, &meta, sizeof(struct ShmFifoMeta), 0); 
  if (!ret) {
    goto SHMFIFO_DO_EXIT;
  }

  if (ret < 0) {
    SHMFIFO_ERR_OUT("load version failed, pread error %d", errno);
    ret = -SHMFIFO_ERR_LOAD_VERSION_PREAD; 
    goto SHMFIFO_DO_EXIT;
  }
  if ((size_t)ret < sizeof(struct ShmFifoMeta)) {
    SHMFIFO_ERR_OUT("load version failed, size error");
    ret = -SHMFIFO_ERR_LOAD_VERSION_SIZE;
  }
SHMFIFO_DO_EXIT:
  *magic = meta.magic;
  *version = meta.version; 

  return SHMFIFO_ERR_NO;
}

static int ShmFifoFileReady(int fd, size_t size)
{
  int         ret = SHMFIFO_ERR_NO;
  uint32_t    magic;
  uint32_t    version;
  struct stat st;

  ret = ShmFifoLoadVersion(fd, &magic, &version);
  if (ret != SHMFIFO_ERR_NO) {
    return SHMFIFO_FALSE;
  }

  if (magic != SHMFIFO_MAGIC || version != SHMFIFO_VERSION) {
    SHMFIFO_DEBUG_OUT("magic %x or version %x error", magic, version);
    return SHMFIFO_FALSE;
  }

  if (fstat(fd, &st) < 0) {
    SHMFIFO_ERR_OUT("fstat failed, err %d", errno);
    return SHMFIFO_FALSE;
  }

  if ((size_t)st.st_size != size) {
    SHMFIFO_ERR_OUT("fifo file size %ld, %lu error", st.st_size, size);
    return SHMFIFO_FALSE;
  }
  
  return SHMFIFO_TRUE;
}

static int ShmFifoFormat(int fd, size_t size)
{
  if (ftruncate(fd, 0) < 0) {
    SHMFIFO_ERR_OUT("fifo format failed, ftruncate error %d", errno);
    return -SHMFIFO_ERR_FORMAT_FTRUNCATE;
  }
  if (ftruncate(fd, size) < 0) {
    SHMFIFO_ERR_OUT("fifo foramt failed, ftruncate error %d", errno);
    return -SHMFIFO_ERR_FORMAT_FTRUNCATE_SIZE;
  }

  return SHMFIFO_ERR_NO;
}

