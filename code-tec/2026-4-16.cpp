#include<omp.h> 
#include<iostream> 
using namespace std;
int main()
{
	int i;
	#pragma omp parallel num_threads(4)
	{
		#pragma omp sections //第1个sections  
		{
			#pragma omp section 
			{
				cout << "section 1 线程ID：" << omp_get_thread_num() << "\n";
			}
			#pragma omp section  
			{
				cout << "section 2 线程ID：" << omp_get_thread_num() << "\n";
			}
		}
		#pragma omp sections //第2个sections  
		{
			#pragma omp section
			{
				cout << "section 3 线程ID：" << omp_get_thread_num() << "\n";
			}
			#pragma omp section
			{
				cout << "section 4 线程ID：" << omp_get_thread_num() << "\n";
			}
		}
	}
	return 0;
}

//#include<omp.h> 
//#include<iostream> 
//using namespace std;
//int main()
//{
//	int i;
//	#pragma omp parallel sections num_threads(4)
//	{
//	#pragma omp section 
//		{
//			cout << "section 1 线程ID：" << omp_get_thread_num() << "\n";
//		}
//	#pragma omp section  
//		{
//			cout << "section 2 线程ID：" << omp_get_thread_num() << "\n";
//		}
//	#pragma omp section
//		{
//			cout << "section 3 线程ID：" << omp_get_thread_num() << "\n";
//		}
//	#pragma omp section
//		{
//			cout << "section 4 线程ID：" << omp_get_thread_num() << "\n";
//		}
//	}
//	return 0;
//}
//#include<omp.h>
//#include<iostream>
//using namespace std;
//int main()
//{
//	#pragma omp parallel
//	{
//		#pragma omp sections nowait
//		{
//			#pragma omp section
//			{
//				cout << "section 1 线程ID：" << omp_get_thread_num() << "\n";
//			}
//			#pragma omp section
//			{
//				cout << "section 2 线程ID：" << omp_get_thread_num() << "\n";
//			}
//		}
//		#pragma omp sections
//		{
//			#pragma omp section
//			{
//				cout << "section 1 线程ID：" << omp_get_thread_num() << "\n";
//			}
//			#pragma omp section
//			{
//				cout << "section 2 线程ID：" << omp_get_thread_num() << "\n";
//			}
//		}
//	}
//	return 0;
//}