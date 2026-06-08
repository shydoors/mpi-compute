#include<omp.h> 
#include <iostream> 
using namespace std; 
//循环测试函数 
void test() 
{  
	float a=1.0;
	for(int i=0;i<100000;i++)  
	{  
		a = a + a * i;
	} 
}
int main(int argc, char * argv[])
{
	cout << "这是一个串行测试程序!\n";  
	double start = omp_get_wtime();//获取起始时间  
	for(int i = 0; i < 100000; i++)  
	{  
		test();  
	}  
	double end = omp_get_wtime( );  
	cout<<"计算耗时为："<<end - start<<"\n";  
	//cin>>end;  
	return 0; 
} 