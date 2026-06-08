#include<omp.h> 
#include<iostream> 
using namespace std;
int main()
{
	int numbers[10] = { 5,7,8,2,4,5,6,7,9,7 };
	int i, j, max = 0;
	#pragma omp parallel for 
	for (i = 0; i < 10; i++) 
	{
		#pragma omp critical (C01)  
		if(max<numbers[i])  
		{  
			j=omp_get_num_threads();//获取线程数目  
			cout<<"线程数目："<<j<<"\n";  
			j=omp_get_thread_num();//当前线程ID  
			cout<<"当前线程ID："<<j<<"\n";  
			max=numbers[i];  
		}  
	}  
	cout<<"最大值为："<<max; 

	return 0;
}