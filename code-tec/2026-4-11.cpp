#include<omp.h> 
#include<iostream> 
using namespace std;
int main() 
{ 
	cout << "lastprivate输出：\n";  
	int i = 0, j = 10;  
    #pragma omp parallel for lastprivate(j)  
	for (i = 0; i < 8; i++) 
	{ 
		j = 5;  
		cout << i << ":" << j << "\n";  
		j++;  
		cout << i << ":" << j << "\n"; 
	}  
	cout << "最后j的值为:" << j << "\n";  
	return 0; 
}