#include <iostream>
#include <fstream>
#include <cstdlib> // 用于 std::exit
// 如果你确实需要跳过文件开头的 K 个字节，应该使用 seekg，而不是 read 0 字节。
// 如果不需要跳过任何字节，请直接删除这个函数。
void SkipBytes(std::ifstream &ifs, std::streamsize bytes_to_skip) {
    if (bytes_to_skip > 0) {
        ifs.seekg(bytes_to_skip, std::ios::cur); // std::ios::cur 表示从当前位置跳过
    }
}
int main() {
    // 1. 统一使用 std:: 命名空间前缀，提高代码可读性
    std::ifstream ifs1("../data/a.dat", std::ios::in | std::ios::binary);
    if (!ifs1) {
        std::cerr << "Error: Cannot open file ./dat/a.dat" << std::endl;
        std::exit(1);
    }
    // 2. 如果你原本想跳过某些字节，取消下面这行的注释并传入字节数。
    // 如果不需要，直接删掉这行。
    // SkipBytes(ifs1, 0);
    double xy[2] = {0.0, 0.0};
    // 3. C++ 读取二进制文件的标准且安全的模式：
    // 直接将 ifs1.read() 作为 while 的条件。
    // read() 会返回流的引用，在布尔上下文中会自动检查流是否处于 good() 状态。
    // 这样完美避免了 !eof() 带来的“多读取一次无效数据”的 bug。
    while (ifs1.read(reinterpret_cast<char*>(xy), sizeof(xy))) {
        std::cout << xy[0] << "," << xy[1] << std::endl;
    }
    // 4. 循环结束后，检查是因为正常到达文件末尾 (EOF)，还是因为文件损坏等错误中断
    if (!ifs1.eof()) {
        std::cerr << "Warning: File reading stopped due to an error before reaching EOF." << std::endl;
    }
    // 5. 虽然 ifstream 析构时会自动 close()，但显式调用也是好习惯
    ifs1.close();
    return 0;
}