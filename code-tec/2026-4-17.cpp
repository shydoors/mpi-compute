#include <iostream>
#include <omp.h>
using namespace std;
int main() {
  int i, k;
  double starttime, endtime;
  cout << "主线程中：" << "\n";
  i = omp_get_num_threads(); // 获取线程数目
  cout << "线程数目：" << i << "\n";
  i = omp_get_max_threads(); // 获取最多线程数目
  cout << "最多线程数目：" << i << "\n";
  i = omp_get_thread_num(); // 当前线程ID
  cout << "当前线程ID：" << i << "\n";
  i = omp_get_num_procs(); // 获取程序可用的处理器数目
  cout << "程序可用的处理器数目：" << i << "\n";
  starttime = omp_get_wtime(); // 获取当前时间
  cout << "程序开始运行时间：" << starttime << "s\n";
  i = omp_in_parallel(); // 程序是否处于并行
  cout << "程序是否处于并行：" << i << "\n";
  i = omp_get_nested(); // 是否允许并行嵌套
  cout << "是否允许并行嵌套：" << i << "\n";
  cout << "\n程序开始并行：\n";
#pragma omp parallel for
  for (i = 0; i < 2; i++) {
    k = omp_in_parallel(); // 程序是否处于并行
    cout << "程序是否处于并行：" << k << "\n";
    k = omp_get_num_threads(); // 获取线程数目
    cout << "线程数目：" << k << "\n";
    k = omp_get_thread_num(); // 当前线程ID
    cout << "当前线程ID：" << k << "\n";
  }
  cout << "\n程序并行结束！\n";
  endtime = omp_get_wtime(); // 获取当前时间
  cout << "程序结束时间：" << endtime << "s\n";
  cout << "程序运行时间：" << endtime - starttime << "s\n";
}