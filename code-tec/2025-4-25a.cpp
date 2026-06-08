#include <iostream>
#include "stdio.h"
#include "omp.h"
using namespace std;
int main()
{
	bool flag = true, change = true;
	int n;
	cin >> n;
	int *a = new int[n];
	for (int i = 0; i < n; i++)
	{
		cin >> a[i];
	}
	//设定并行线程数
	omp_set_num_threads(n / 2);
	//flag为true或者 change为false（即要进行奇排序时）进入循环
	while (flag || !change)
	{
		flag = false;

		if (change) //偶循环
		{
			//并行for开始
			#pragma omp parallel for
			for (int i = 0; i < n - 1; i += 2)
			{
				if (a[i] > a[i + 1])
				{
					flag = true;
					int temp = a[i];
					a[i] = a[i + 1];
					a[i + 1] = temp;
				}
			}

		}
		else //奇循环
		{
			//并行for开始
			#pragma omp parallel for
			for (int i = 1; i < n - 1; i += 2)
			{
				if (a[i] > a[i + 1])
				{
					flag = true;
					int temp = a[i];
					a[i] = a[i + 1];
					a[i + 1] = temp;
				}
			}
		}
		//奇变偶 偶变奇
		if (change)
		{
			change = false;
		}
		else
		{
			change = true;
		}
	}
	for (int i = 0; i < n; i++)
	{
		cout << a[i] << " ";
	}
	return 0;
}