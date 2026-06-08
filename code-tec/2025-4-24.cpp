#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

void BubbleSort(int * pData, int nSize)
{
	if (!pData)
	{
		printf("pData is null\n");
		return;
	}
	for (int i = 0; i < nSize; i++)
	{
		for (int j = 0; j < nSize - i - 1; j++)
		{
			if (pData[j] > pData[j + 1])
			{
				int nTmp = pData[j];
				pData[j] = pData[j + 1];
				pData[j + 1] = nTmp;
			}
		}
	}
}
void main()
{
	int * pData = NULL;
	int nNums = 0;
	printf("please input num count:");
	scanf("%d", &nNums);
	printf("you input nums:%d\n", nNums);

	pData = new int[nNums];

	for (int i = 0; i < nNums; i++)
	{
		pData[i] = rand();
	}

	for (int i = 0; i < nNums; i++)
	{
		if (i && i % 10 == 0)
		{
			printf("\n");
			printf("%d	", pData[i]);
		}
		else
		{
			printf("%d	", pData[i]);
		}
	}

	printf("\n");

	printf("sort after\n");

	DWORD dwStart = GetTickCount();

	BubbleSort(pData, nNums);

	DWORD dwEnd = GetTickCount();

	for (int i = 0; i < nNums; i++)
	{
		if (i && i % 10 == 0)
		{
			printf("\n");
			printf("%d	", pData[i]);
		}
		else
		{
			printf("%d	", pData[i]);
		}
	}

	printf("\n");

	printf("sort time:%d msec\n", dwEnd - dwStart);

	if (pData)
	{
		delete[] pData;
		pData = NULL;
	}

	printf("\n");

	system("pause");
}
