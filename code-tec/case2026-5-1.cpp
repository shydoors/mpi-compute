#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

/**
 * @brief 计算一维卷积（朴素实现，时间复杂度 O(n*m)）
 * @param signal 输入信号
 * @param kernel 卷积核
 * @return 卷积结果向量
 */
std::vector<int> convolve(const std::vector<int> &signal,
                          const std::vector<int> &kernel) {
  int signal_length = static_cast<int>(signal.size());
  int kernel_length = static_cast<int>(kernel.size());
  int output_length = signal_length + kernel_length - 1;
  std::vector<int> output(output_length, 0); // 初始化输出为0
  // 卷积核心计算
  for (int i = 0; i < signal_length; ++i) {
    for (int j = 0; j < kernel_length; ++j) {
      output[i + j] += signal[i] * kernel[j];
    }
  }
  return output;
}
/**
 * @brief 生成伪随机测试信号（可复现）
 */
std::vector<int> generateSignal(int length, unsigned int seed = 42) {
  std::vector<int> signal(length);
  std::mt19937 gen(seed); // Mersenne Twister 随机引擎
  std::uniform_int_distribution<> dis(-50, 49);
  for (int &val : signal) {
    val = dis(gen);
  }
  return signal;
}
/**
 * @brief 生成简单卷积核 [1,1,...,0,0,...,-1,-1,...]
 */
std::vector<int> generateKernel(int length) {
  std::vector<int> kernel(length);
  int third = length / 3;
  for (int i = 0; i < length; ++i) {
    if (i < third) {
      kernel[i] = 1;
    } else if (i < 2 * third) {
      kernel[i] = 0;
    } else {
      kernel[i] = -1;
    }
  }
  return kernel;
}
int main() {
  // ========== 配置参数（constexpr 编译期常量）==========
  constexpr int SIGNAL_LENGTH = 10000000;  // 信号长度，可调整
  constexpr int KERNEL_LENGTH = 15;        // 卷积核长度
  constexpr unsigned int RANDOM_SEED = 42; // 随机种子
  // ===================================================
  // 使用 std::vector 自动管理内存（RAII）
  auto signal = generateSignal(SIGNAL_LENGTH, RANDOM_SEED);
  auto kernel = generateKernel(KERNEL_LENGTH);
  // ========== 高精度计时（std::chrono）==========
  auto start_time = std::chrono::high_resolution_clock::now();
  // 执行卷积计算
  auto output = convolve(signal, kernel);
  auto end_time = std::chrono::high_resolution_clock::now();
  // =============================================
  // 计算耗时（微秒精度）
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);
  double elapsed_seconds = duration.count() / 1'000'000.0;
  // ========== 输出结果 ==========
  std::cout << "===== 卷积计算完成 =====\n";
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "信号长度: " << SIGNAL_LENGTH << "\n";
  std::cout << "卷积核长度: " << KERNEL_LENGTH << "\n";
  std::cout << "输出长度: " << output.size() << "\n";
  std::cout << "程序运行时间: " << elapsed_seconds << " 秒 ("
            << duration.count() << " μs)\n";
  // 打印部分结果验证（避免终端刷屏）
  std::cout << "\n输出结果前10个值: ";
  for (size_t i = 0; i < 10 && i < output.size(); ++i) {
    std::cout << output[i] << " ";
  }
  std::cout << "...\n";
  std::cout << "输出结果后10个值: ";
  for (size_t i = output.size() - 10; i < output.size(); ++i) {
    std::cout << output[i] << " ";
  }
  std::cout << "\n";
  // std::vector 自动析构，无需手动释放内存
  return 0;
}