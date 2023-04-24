#ifndef TEST_MSG_H_
#define TEST_MSG_H_
struct TestMsg {
  uint64_t ts;
  uint64_t seq;
  uint32_t f1;
  uint16_t f2;
  uint8_t  f3;
  char     data[0];
}__attribute__((packed));


#endif
