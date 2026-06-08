#include<omp.h> 
#include<iostream> 
using namespace std;
int main()
{
	#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < 12; i++)
	{
		printf("Thread: %d, Iteration: %d\n", omp_get_thread_num(), i);
	}
	return 0;
}
