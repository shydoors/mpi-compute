/**
 * @file 2025-4-24a.cpp
 * @brief OpenMP 并行冒泡排序（分治合并）
 */
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <omp.h>

#define NUM_DATA 43

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

/**
 * @brief 归并两个有序数组
 * @param pInput1 第一个有序数组
 * @param nInput1Len 第一个数组长度
 * @param pInput2 第二个有序数组
 * @param nInput2Len 第二个数组长度
 * @return 合并后的有序数组（需调用者 delete[]）
 */
int *merge(int *pInput1, int nInput1Len, int *pInput2, int nInput2Len) {
  if (!pInput1 || !pInput2) {
    return NULL;
  }
  int nOutLen = nInput1Len + nInput2Len;
  int *pOut = new int[nOutLen];
  if (pOut == NULL) {
    return NULL;
  }
  int i = 0;
  int j = 0;
  int m = 0;
  while (i < nInput1Len && j < nInput2Len) {
    if (pInput1[i] < pInput2[j]) {
      pOut[m++] = pInput1[i];
      i++;
    } else {
      pOut[m++] = pInput2[j];
      j++;
    }
  }
  while (i < nInput1Len) {
    pOut[m++] = pInput1[i++];
  }
  while (j < nInput2Len) {
    pOut[m++] = pInput2[j++];
  }
  return pOut;
}

/**
 * @brief OpenMP 并行排序（分块排序 + 归并）
 */
void BubbleSort2(int *pData, int nSize) {
  if (!pData) {
    printf("pData is null\n");
    return;
  }
  int nNums = nSize / 4;
  int nStart = 0;
  int nEnd = 0;
#pragma omp parallel num_threads(4) private(nStart, nEnd)
  {
#pragma omp sections
    {
#pragma omp section
      {
        printf("section 1 ThreadId = %d\n", omp_get_thread_num());
        nStart = 0;
        nEnd = nStart + nNums;
        BubbleSort(pData + nStart, nNums);
      }
#pragma omp section
      {
        printf("section 2 ThreadId = %d\n", omp_get_thread_num());
        nStart = nNums;
        nEnd = nStart + nNums;
        BubbleSort(pData + nStart, nNums);
      }
#pragma omp section
      {
        printf("section 3 ThreadId = %d\n", omp_get_thread_num());
        nStart = nNums * 2;
        nEnd = nStart + nNums;
        BubbleSort(pData + nStart, nNums);
      }
#pragma omp section
      {
        printf("section 4 ThreadId = %d\n", omp_get_thread_num());
        nStart = nNums * 3;
        nEnd = nStart + nNums;
        BubbleSort(pData + nStart, nNums);
      }
    }
  }
  nStart = nNums * 4;
  nEnd = nSize - 1;
  BubbleSort(pData + nStart, nEnd - nStart + 1);
  int *pData1 = NULL;
  int *pData2 = NULL;
#pragma omp parallel num_threads(2)
  {
#pragma omp sections
    {
#pragma omp section
      {
        pData1 = merge(pData, nNums, pData + nNums, nNums);
      }
#pragma omp section
      {
        pData2 = merge(pData + nNums * 2, nNums, pData + nNums * 3, nNums);
      }
    }
  }
  if (!pData1 || !pData2) {
    delete[] pData1;
    delete[] pData2;
    printf("merge1 failed.\n");
    return;
  }
  int *pData3 = merge(pData1, nNums * 2, pData2, nNums * 2);
  delete[] pData1;
  delete[] pData2;
  if (!pData3) {
    printf("merge2 failed.\n");
    return;
  }
  int *pData4 = merge(pData3, nNums * 4, pData + nNums * 4, nSize - nNums * 4);
  delete[] pData3;
  if (!pData4) {
    printf("merge3 failed.\n");
    return;
  }

#pragma omp parallel for
  for (int i = 0; i < nSize; i++) {
    pData[i] = pData4[i];
  }
  delete[] pData4;
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

  clock_t dwStart = clock();
  BubbleSort2(pData, nNums);
  clock_t dwEnd = clock();

  printf("sort after:\n");
  for (int i = 0; i < nNums; i++) {
    if (i && i % 10 == 0) {
      printf("\n");
    }
    printf("%d\t", pData[i]);
  }
  printf("\n");
  printf("sort time:%ld ms\n", (dwEnd - dwStart) * 1000 / CLOCKS_PER_SEC);
  delete[] pData;
  return 0;
}
