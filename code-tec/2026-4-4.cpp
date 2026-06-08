#include<omp.h> 
#include <iostream> 
using namespace std;
// 循环测试函数 
void test()
{
    float a = 1.0;
    for (int i = 0; i < 5000000; i++)
    {
        for (int j = 0; j < 1000; j++)
        {
            a = a + a * i;
        }
    }
}
int main(int argc, char* argv[])
{
    cout << "这是一个新的并行测试程序!\n";

    while (true)
    {
        cout << "请输入并行次数（输入0结束）：\n";
        int N = 1;
        cin >> N;

        // 输入0时退出程序
        if (N == 0)
        {
            cout << "程序结束。\n";
            break;
        }

        cout << "开始进行计算...\n";
        double start = omp_get_wtime();  // 获取起始时间  
        #pragma omp parallel for 
        for (int i = 0; i < N; i++)
        {
            test();
        }

        double end = omp_get_wtime();
        cout << "计算耗时为：" << end - start << " 秒\n";
        cout << "------------------------\n";
    }
    return 0;
}