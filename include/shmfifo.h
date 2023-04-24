#ifndef SHMFIFO_H_
#define SHMFIFO_H_

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
struct ShmFifo;

#define SHMFIFO_MODE_READ  (1)
#define SHMFIFO_MODE_WRITE (2)

struct ShmFifo* ShmFifoOpen(const char *path, size_t msg_size, size_t msg_count);
void ShmFifoClose(struct ShmFifo *fifo);
ssize_t ShmFifoPush(struct ShmFifo *fifo, const char* buf, const size_t buf_size);
int ShmFifoPop(struct ShmFifo *fifo);
ssize_t ShmFifoPopData(struct ShmFifo *fifo,  char* const buf, const size_t buf_size);
void* ShmFifoTop(struct ShmFifo *fifo, size_t* const size);
#ifdef __cplusplus
}
#endif
#endif
