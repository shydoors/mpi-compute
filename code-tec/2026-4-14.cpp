#include <iostream>
#include <omp.h>
using namespace std;

int main() {
  cout << "=== ScheduleTest 输出 ===\n";
  while (true) {
    int i = 0, j, chunkSize = 1;
    double starttime, endtime;
    cout << "\n请输入并行块的大小 chunkSize（输入 0 结束）：\n";
    cin >> chunkSize;
    // 输入 0 时退出程序
    if (chunkSize == 0) {
      cout << "程序结束。\n";
      break;
    }
    // 可选：防止非法输入
    if (chunkSize < 0 || chunkSize > 200) {
      cout << "提示：chunkSize 建议在 [1, 200] 范围内，继续执行...\n";
    }
    starttime = omp_get_wtime();
#pragma omp parallel for private(j) schedule(static, chunkSize)
    for (i = 0; i < 200; i++) {
      for (j = 0; j < 100000000; j++)
        ; // 空循环，用于模拟计算负载
    }

    endtime = omp_get_wtime();
    cout << "chunkSize = " << chunkSize << " | 计算耗时："
         << endtime - starttime << " s\n";
    cout << "------------------------\n";
  }

  return 0;
}