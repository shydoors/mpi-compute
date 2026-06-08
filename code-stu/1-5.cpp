#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdlib.h>
/**
 * 正常的串行高斯求和
 */
int main(int32_t argc, char *argv[]) {
  std::int32_t max_num = strtol(argv[1], NULL, 10);
  std::int32_t sum = 0;
  for (std::int32_t i = 0; i <= max_num; ++i) {
    sum += i;
  }
  std::cout<<"Sum  = "<<sum<<std::endl;
  return 0;
}