#include "HSAlgo.h"

int main(int argc, char* argv[]) {
  std::vector<std::size_t> ids{2, 533, 6, 9, 10, 40, 76};
  HS::HSAlgo test(ids);
  test.Run();
  return 0;
}
