#include <iostream>
#include <vector>
using namespace std;

void oddEvenSort(vector<int> &arr) {
  bool sorted = false; // 标记数组是否已排序
  int n = arr.size();

  while (!sorted) {
    sorted = true; // 假设本轮排序后数组已排序

    // 奇数轮：处理奇数索引
    for (int i = 1; i < n - 1; i += 2) {
      if (arr[i] > arr[i + 1]) {
        swap(arr[i], arr[i + 1]); // 交换相邻元素
        sorted = false;           // 如果有交换发生，说明数组尚未排序
      }
    }

    // 偶数轮：处理偶数索引
    for (int i = 0; i < n - 1; i += 2) {
      if (arr[i] > arr[i + 1]) {
        swap(arr[i], arr[i + 1]); // 交换相邻元素
        sorted = false;           // 如果有交换发生，说明数组尚未排序
      }
    }
  }
}

int main() {
  vector<int> arr = {34, 2, 23, 67, 100, 88, 1, 56};
  cout << "Original array: ";
  for (int num : arr) {
    cout << num << " ";
  }
  cout << endl;

  oddEvenSort(arr);

  cout << "Sorted array: ";
  for (int num : arr) {
    cout << num << " ";
  }
  cout << endl;

  return 0;
}