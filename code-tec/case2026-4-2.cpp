//2并行验证
#include <iostream>
#include <vector>
#include <mpi.h>

using namespace std;

void oddEvenSort(vector<int>& arr) {
	bool sorted = false; // 标记数组是否已排序
	int n = arr.size();

	while (!sorted) {
		sorted = true; // 假设本轮排序后数组已排序

		 //奇数轮：处理奇数索引
		for (int i = 1; i < n - 1; i += 2) {
			if (arr[i] > arr[i + 1]) {
				swap(arr[i], arr[i + 1]); // 交换相邻元素
				sorted = false; // 如果有交换发生，说明数组尚未排序
			}
		}

		 //偶数轮：处理偶数索引
		for (int i = 0; i < n - 1; i += 2) {
			if (arr[i] > arr[i + 1]) {
				swap(arr[i], arr[i + 1]); // 交换相邻元素
				sorted = false; // 如果有交换发生，说明数组尚未排序
			}
		}
	}
}

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	vector<int> arr;
	vector<int> local_arr;
	int n = 8; // 假设数组大小为8

	if (rank == 0) {
		arr = { 34, 2, 23, 67, 100, 88, 1, 56 };
		cout << "Original array: ";
		for (int num : arr) {
			cout << num << " ";
		}
		cout << endl;
	}

	 //每个进程处理的数据量
	int local_size = n / size;
	local_arr.resize(local_size);

	 //将数组分割并分发到各个进程
	MPI_Scatter(arr.data(), local_size, MPI_INT, local_arr.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);

	 //每个进程对自己的部分进行排序
	oddEvenSort(local_arr);

	 //收集各个进程的排序结果
	MPI_Gather(local_arr.data(), local_size, MPI_INT, arr.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank == 0) {
		 //主进程对所有部分进行最终的排序
		oddEvenSort(arr);

		cout << "Sorted array: ";
		for (int num : arr) {
			cout << num << " ";
		}
		cout << endl;
	}

	MPI_Finalize();
	return 0;
}