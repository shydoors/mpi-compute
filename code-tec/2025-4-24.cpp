/**
 * @file 2025-4-24.cpp
 * @brief 冒泡排序（串行版本）
 */
#include <cstdio>
#include <cstdlib>
#include <ctime>

/**
 * @brief 冒泡排序
 * @param pData 待排序数组指针
 * @param nSize 数组大小
 */
void BubbleSort(int *pData, int nSize) {
    if (!pData) {
		printf("pData is null\n");
		return;
	}
    for (int i = 0; i < nSize; i++) {
        for (int j = 0; j < nSize - i - 1; j++) {
            if (pData[j] > pData[j + 1]) {
				int nTmp = pData[j];
				pData[j] = pData[j + 1];
				pData[j + 1] = nTmp;
			}
		}
	}
}

int main() {
    int *pData = NULL;
	int nNums = 0;
	printf("please input num count:");
	scanf("%d", &nNums);
	printf("you input nums:%d\n", nNums);

	pData = new int[nNums];
    for (int i = 0; i < nNums; i++) {
		pData[i] = rand();
	}

    for (int i = 0; i < nNums; i++) {
        if (i && i % 10 == 0) {
			printf("\n");
		}
        printf("%d\t", pData[i]);
		}
	printf("\n");
	printf("sort after\n");
    clock_t dwStart = clock();
	BubbleSort(pData, nNums);
    clock_t dwEnd = clock();
    for (int i = 0; i < nNums; i++) {
        if (i && i % 10 == 0) {
			printf("\n");
		}
        printf("%d\t", pData[i]);
		}
	printf("\n");
    printf("sort time:%ld ms\n", (dwEnd - dwStart) * 1000 / CLOCKS_PER_SEC);
		delete[] pData;
	printf("\n");
    return 0;
}

