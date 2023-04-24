#include <string>
#include <string.h>

#include "shmfifo.h"
#include "shmfifo_error.h"
#include "shmfifo_log.h"
#include "test_msg.h"

static void MakeTestData(char data[], size_t size);
int TestPush(std::string fifo_name, int n)
{
  struct ShmFifo    *fifo;
  struct TestMsg *msg;
  size_t          msg_size;
  int             i; 
  int             ret = SHMFIFO_ERR_NO;
  
  fifo = ShmFifoOpen(fifo_name.c_str(), 1024, 32);  
  if (!fifo) {
    return -SHMFIFO_ERR_OPEN;
  }
  msg_size = sizeof(struct TestMsg) + 10;
  msg = (struct TestMsg *)malloc(msg_size);
  memset(msg, 0, msg_size);
  MakeTestData(msg->data, 10);
  for (i = 0; i < n; ++i) {
    msg->seq++;
    ret = ShmFifoPush(fifo, (const char *)msg, msg_size);
    if (ret < 0) {
      SHMFIFO_ERR_OUT("ShmFifoPush err %d", ret);
      return ret;
    }
  } 
  ShmFifoClose(fifo);
  SHMFIFO_DEBUG_OUT("ShmFifoPush msg %d", i);
  return ret;
}

static void MakeTestData(char data[], size_t size)
{
  memset(data, 0, size);
  for (int i = 0; i < (int)size; ++i) {
    data[i] = '@' + i;
  }
  data[0] = '{';
  data[size - 1] = '}';
  //printf("%s\n", data);
}
