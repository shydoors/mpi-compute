#include <cstdlib>
#include <fstream>
#include <iostream>

int main() {
  // 1. 打开文件
  std::ifstream ifs("../../data/a.dat", std::ios::in | std::ios::binary);
  if (!ifs) {
    std::cerr << "Error: Cannot open file ../../data/a.dat" << std::endl;
    return 1; // 推荐使用 return 代替 exit()，以便正确触发局部对象的析构
  }

  // 2. 如果需要跳过开头的 K 个字节，请使用 seekg (彻底废弃 DisCardKByte)
  // const std::streamsize K = 10;
  // ifs.seekg(K, std::ios::cur);
  double xy[2] = {0.0, 0.0};
  // 3. 核心重构：直接将 read() 放在 while 条件中
  // read() 返回流的引用，在布尔上下文中会自动检查 failbit 和 badbit。
  // 当读取失败（包括到达 EOF 导致读取字节数不足）时，循环自动终止。
  // 这完美避免了 !eof() 带来的“多执行一次循环体”的问题。
  while (ifs.read(reinterpret_cast<char *>(xy), sizeof(xy))) {
    std::cout << xy[0] << "," << xy[1]
              << "\n"; // 使用 \n 代替 endl 提高 I/O 性能
  }
  /**
   * 循环结束后，检查是正常 EOF 还是发生了读取错误
   */
  if (!ifs.eof()) {
    std::cerr << "Warning: Read error occurred before reaching EOF."
              << std::endl;
  }
  /**
   * ifs 离开作用域时会自动调用析构函数关闭文件，无需显式 close()
   */
  return 0;
}