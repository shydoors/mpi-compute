#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
/**
 * 串行计算 Pi
 */
int main(int argc, char *argv[]) {
  std::int64_t n;
  std::cin >> n;
  auto start = std::chrono::high_resolution_clock::now();
  double factor = 1.0;
  double sum = 0.0;
  for (std::int64_t i = 0; i < n; i++) {
    sum += factor / ((2 * i + 2) * (2 * i + 3) * (2 * i + 4));
    factor = -factor;
  }
  sum = 3.0 + 4.0 * sum;
  std::cout << std::fixed << std::setprecision(14);
  std::cout << "With n = " << n << " terms,\n";
  std::cout << " pi = " << sum << " by calculation \n"
            << " pi = " << 4.0 * atan(1.0) << " by 4.0 * atan(1.0)"
            << std::endl;
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "It cost  " << duration.count() << " ms" << std::endl;
  return 0;
}
