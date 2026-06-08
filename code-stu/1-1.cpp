#include <cstdint>
#include <cstdlib>
#include <iostream>
int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <number>\n";
    return 1;
  }
  std::int32_t num_t = std::strtol(argv[1], nullptr, 10);
  std::cout << " The serial program says hello to you with input " << num_t
            << "\n";
  return 0;
}