#include <fstream>
#include <iostream>
#include "HSAlgo.h"
#include "log_util.h"

int main(int argc, char* argv[]) {
  std::string line;
  std::ifstream test_file(argv[1]);
  std::vector<std::size_t> ids;

  if (argc != 2) {
    std::cerr << "Wrong parameter number: " << argc - 1 << std::endl;
    return -1;
  }

  if (!test_file.is_open()) {
    std::cerr << "Wrong test file path: " << argv[1] << std::endl;
    return -1;
  }

  while (std::getline(test_file, line)) {
    if (IsNumber(line)) {
      ids.push_back(std::stoul(line));
    }
  }
  test_file.close();

  HS::HSAlgo hs_algorithm(ids);
  hs_algorithm.Run();
  return 0;
}
