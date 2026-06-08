#include<omp.h> 
#include <iostream> 
using namespace std;
//函数fx 
double function01(int i,int n) 
{  
	double x=(double)(i+1)/n;  
	return 1/(1+x*x); 
} 
int main(int argc, char * argv[])
{
	int n;  
	cout << "请输入计算次数：\n";  
	cin >> n;
	double total = 0;  
	double startTime = omp_get_wtime();
	#pragma omp parallel for reduction(+:total) 
	for (int i = 0; i < n; i++) 
	{ 
		total = total + function01(i, n); 
	}
	double endTime = omp_get_wtime();  
	total = total * 4 / n;  
	cout << "计算结果为：" << total << "\n";  
	cout << "计算所需时间：" << endTime - startTime << "\n";
	return 0;
}