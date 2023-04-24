#include "test_pop.h"
#include "test_msg.h"

#include "shmfifo.h"
#include "shmfifo_log.h"
#include "shmfifo_error.h"

void dump_msg(struct TestMsg* msg, size_t size)
{
  SHMFIFO_DEBUG_OUT("===============================");
  SHMFIFO_DEBUG_OUT("msg %p, size %lu", msg, size);
  SHMFIFO_DEBUG_OUT("ts %lu", msg->ts);
  SHMFIFO_DEBUG_OUT("seq %lu", msg->seq);
  SHMFIFO_DEBUG_OUT("f1 %u", msg->f1);
  SHMFIFO_DEBUG_OUT("f2 %u", msg->f2);
  SHMFIFO_DEBUG_OUT("f3 %u", msg->f3);
  SHMFIFO_DEBUG_OUT("data %s", msg->data);
  SHMFIFO_DEBUG_OUT("===============================");
}

int TestPop(std::string fifo_name)
{
  int             ret = SHMFIFO_ERR_NO;
  int             i = 0;
  struct TestMsg *msg = NULL;
  size_t          size;
  struct ShmFifo    *fifo;

  fifo = ShmFifoOpen(fifo_name.c_str(), 1024, 32);
  if (!fifo) {
    return -SHMFIFO_ERR_OPEN;
  }

  do {
    msg = (struct TestMsg *)ShmFifoTop(fifo, &size);
    if (!msg) {
      break;
    }
    dump_msg(msg, size);
    ret = ShmFifoPop(fifo);
    if (ret != SHMFIFO_ERR_NO) {
      SHMFIFO_ERR_OUT("ShmFifoPop failed, err %d", ret);
      goto TEST_POP_OUT;
    }
    ++i;
  } while (1);

TEST_POP_OUT:
  ShmFifoClose(fifo);
  SHMFIFO_DEBUG_OUT("total pop msg %d", i);  
  return ret;
}
