#include <string>
#include <libgen.h>

#include "test_pop.h"
#include "test_push.h"

int main(int argc, char *argv[])
{
  std::string prog = basename(argv[0]);
  if (prog == "test_pop") {
    return TestPop(argv[1]);
  }

  if (prog == "test_push") {
    return TestPush(argv[1], atoi(argv[2]));
  }
  return 0;
}
