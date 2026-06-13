/**
 * @file 2025-4-25.cpp
 * @brief 奇偶排序（Odd-Even Sort）
 */
#include <cstdbool>
#include <cstdio>
#include <cstdlib>

/**
 * @brief 交换两个整数
 */
void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

/**
 * @brief 打印数组
 */
void printArray(int a[], int count) {
    for (int i = 0; i < count; i++) {
		printf("%d ", a[i]);
}
    printf("\n");
}

/**
 * @brief 奇偶排序
 */
void Odd_even_sort(int a[], int size) {
	bool sorted = false;
    while (!sorted) {
		sorted = true;
        for (int i = 1; i < size - 1; i += 2) {
            if (a[i] > a[i + 1]) {
				swap(&a[i], &a[i + 1]);
				sorted = false;
			}
		}
        for (int i = 0; i < size - 1; i += 2) {
            if (a[i] > a[i + 1]) {
				swap(&a[i], &a[i + 1]);
				sorted = false;
			}
		}
	}
}
int main(void) {
    int a[] = {3, 5, 1, 6, 9, 7, 8, 0, 11};
	int n = sizeof(a) / sizeof(*a);
	Odd_even_sort(a, n);
	printArray(a, n);
	return 0;
}