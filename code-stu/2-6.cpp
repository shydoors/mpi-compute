#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <omp.h>
int main()
{
    std::int64_t n, i;
    std::int32_t cnt;
    double factor, sum = 0.0;
    n = 100, cnt = 10;
    auto start = std::chrono::high_resolution_clock::now();
    // OpenMP 并行指令参数详解：
    // #pragma omp parallel for : 指示编译器开启并行区域，并自动将紧随其后的 for 循环迭代分发给多个线程执行
    // num_threads(cnt)         : 指定并行区域中创建的线程数量
    // default(none)            : 禁用隐式数据共享规则，强制要求显式声明并行区内所有变量的作用域，提升代码安全性
    // reduction(+:sum)         : 指定 sum 为加法归约变量。各线程使用局部副本累加，循环结束后自动合并至全局 sum
    // private(factor)          : 指定 factor 为线程私有变量。每个线程拥有独立副本，避免多线程写入冲突（数据竞争）
    // shared(n)                : 指定 n 为共享变量。所有线程共同读取同一份内存中的 n 值，用于循环终止条件判断
    #pragma omp parallel for num_threads(cnt) default(none) reduction(+:sum) private(factor) shared(n)
    for (i = 0; i < n; ++i)
    {
        factor = (i % 2 == 0) ? 1.0 : -1.0;
        sum += factor / ((2 * i + 2) * (2 * i + 3) * (2 * i + 4));
    }
    sum = 3.0 + 4.0 * sum;
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << std::fixed << std::setprecision(16);
    std::cout << "With n = " << n << " terms and " << cnt << " threads,\n";
    std::cout << ">>>> Compare of the 2 estimates" << std::endl;
    std::cout << "pi = " << sum << " (calculated) " << std::endl;
    std::cout << "pi = " << 4.0 * atan(1.0) << " (4.0 * atan(1.0)) " << std::endl;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << " s" << std::endl;
    return 0;
}

/**
 * 1. num_threads(cnt) 指定了并行区域内使用的线程数量为 cnt
 * 2. default(none) 规定了并行区域内的变量必须显式地指定它们的共享属性
 * 3. reduction(+ : sum) 指定了 sum
 * 变量在并行区域内进行求和操作的归约，这样每个线程都会有一个私有的 sum 变量，
 * 最后会将它们的值合并到一个全局的 sum 变量中，避免了数据竞争
 * 4. private(factor) 指定了 factor
 * 变量在并行区域内是私有的，每个线程都会有一个独立的 factor 变量，
 * 这样每个线程在计算 factor 的值时不会互相干扰，避免了数据竞争
 */
