#include<omp.h> 
#include<iostream> 
using namespace std;
int main()
{
	omp_lock_t lock;  
	int numbers[10] = { 5,7,8,2,4,5,6,7,9,7 };  
	int i, j, max = 0;  
	omp_init_lock(&lock);//初始化lock  
	#pragma omp parallel for 
	for (i = 0; i < 10; i++) 
	{
		omp_set_lock(&lock);//设置lock  
		if(max<numbers[i])  
		{  
			j=omp_get_num_threads();//获取线程数目  
			cout<<"线程数目："<<j<<"\n";  
			j=omp_get_thread_num();//当前线程ID  
			cout<<"当前线程ID："<<j<<"\n";  
			max=numbers[i];  
		}  
		omp_unset_lock(&lock);//复原lock  
	}  omp_destroy_lock(&lock);//释放loc
	cout << "最大值为：" << max << "\n";
	return 0;
}