#include <cstdint>
#include <omp.h>
#include<iostream>
/**
* 反面案例--高斯求和
* 变量 sum 在主线程中声明为 int32_t sum = 0，是所有线程共享的全局变量
* 所有线程都在执行 sum += i 操作，这实际上是三个操作的组合：
* 1. 读取 sum 的当前值
* 2. 计算 sum + i
* 3. 将结果写回 sum
* 当多个线程同时执行这些操作时，会出现读-修改-写冲突，导致某些加法操作被覆盖
*
* 本质原因是 parallel是默认共享所有变量，导致修改冲突
*/
int main(int argc, char* argv[])
{
    std::int32_t max_num = strtoll(argv[1], NULL, 10);
    std::int32_t num_t = strtoll(argv[2], NULL, 10);
    std::int32_t sum = 0, cnt = 0;
    std::int32_t item = max_num / num_t;
#pragma omp parallel num_threads(num_t)
    {
        std::int32_t id = omp_get_thread_num();
        cnt = omp_get_num_threads();
        for (std::int32_t i = item * id + 1; i <= (id == cnt ? max_num : item * (id + 1)); ++i)
        {
            sum += i;
        }
        std::cout << "id = " << id << " sum= " << sum << std::endl;
    }
    std::cout << "Sum = " << sum << std::endl;
    std::cout << "Total threads = " << cnt << std::endl;
    return 0;
}
