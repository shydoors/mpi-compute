#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define NUM_DATA 43
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
int *merge(int *pInput1, int nInput1Len, int *pInput2, int nInput2Len) {
  int *pOut = NULL;
  int nOutLen = 0;
  if (!pInput1 || !pInput2) {
    return NULL;
  }
  nOutLen = nInput1Len + nInput2Len;
  pOut = new int[nOutLen];
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
    if (pData1)
      delete[] pData1;
    if (pData2)
      delete[] pData2;

    printf("merge1 failed.\n");
    return;
  }

  int *pData3 = NULL;

  pData3 = merge(pData1, nNums * 2, pData2, nNums * 2);

  if (pData1)
    delete[] pData1;
  if (pData2)
    delete[] pData2;

  if (!pData3) {
    printf("merge2 failed.\n");
    return;
  }

  int *pData4 = NULL;

  pData4 = merge(pData3, nNums * 4, pData + nNums * 4, nSize - nNums * 4);

  if (pData3)
    delete[] pData3;

  if (!pData4) {
    printf("merge3 failed.\n");
    return;
  }

#pragma omp parallel for
  for (int i = 0; i < nSize; i++) {
    pData[i] = pData4[i];
  }

  if (pData4) {
    delete[] pData4;
  }

  return;
}

void main() {
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
      printf("%d	", pData[i]);
    } else {
      printf("%d	", pData[i]);
    }
  }

  printf("\n");

  DWORD dwStart = GetTickCount();

  BubbleSort2(pData, nNums);

  DWORD dwEnd = GetTickCount();

  printf("sort after:\n");

  for (int i = 0; i < nNums; i++) {
    if (i && i % 10 == 0) {
      printf("\n");
      printf("%d	", pData[i]);
    } else {
      printf("%d	", pData[i]);
    }
  }

  printf("\n");

  printf("sort time:%d msec\n", dwEnd - dwStart);

  if (pData)
    delete[] pData;

  system("pause");
}
