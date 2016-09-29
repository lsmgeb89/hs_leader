#include "HSAlgo.h"

int main(int argc, char* argv[]) {
  std::vector<std::size_t> ids{2, 6, 9, 5, 3, 7, 8};
  HS::HSAlgo test(ids);
  test.Run();
  return 0;
}
