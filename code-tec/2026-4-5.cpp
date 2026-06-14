#include <iostream>
#include <omp.h>
using namespace std;

// 循环测试函数
void test(int space) {
  float a = 1.0;
  for (int i = 0; i < space; i++) {
    for (int j = 0; j < 10; j++) {
      a = a + a * i;
    }
  }
}
int main(int argc, char *argv[]) {
  const int total = 36288000; // 总计算量固定
  cout << "在总计算量一定的情况下测试程序!\n";
  while (true) {
    cout << "请输入并行次数（输入0结束）：\n";
    int N = 1;
    cin >> N;

    // 输入0时退出程序
    if (N == 0) {
      cout << "程序结束。\n";
      break;
    }
    // 防止除以0或N过大导致space为0
    if (N > total) {
      cout << "错误：并行次数不能超过总计算量 " << total << "，请重新输入。\n";
      continue;
    }

    int space = total / N;
    cout << "每次并行计算量 space = " << space << "\n";
    cout << "开始进行计算...\n";

    double start = omp_get_wtime(); // 获取起始时间

#pragma omp parallel for
    for (int i = 0; i < N; i++) {
      test(space);
    }

    double end = omp_get_wtime();
    cout << "计算耗时为：" << end - start << " 秒\n";
    cout << "------------------------\n";
  }
  return 0;
}