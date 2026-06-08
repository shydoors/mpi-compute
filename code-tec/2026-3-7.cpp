//集合通信
#include <math.h>
#include <mpi.h>
#include <iostream>
using namespace std;
double fun(double x);
double Trap(double left_endpt, double right_endpt, int trap_count, double base_len);
int main(int argc, char *argv[])
{
	int my_rank, comm_sz, n, local_n, source;
	double a, b, h, local_a, local_b, local_int, total_int;
	a = 0;
	b = 1.0;
	n = 1024 * 1024 * 1024;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	double t1 = MPI_Wtime(); /*得到当前时间t1*/
	h = (b - a) / (double)n; // h是每个区间的大小
	local_n = n / comm_sz;   // 每个进程处理的梯形数量

	// 计算每个进程的子区间
	local_a = a + my_rank * local_n * h;
	local_b = local_a + local_n * h;

	// 如果是最后一个进程，处理剩余的梯形
	if (my_rank == comm_sz - 1) {
		local_n += n % comm_sz; // 将剩余的梯形分配给最后一个进程
		local_b = b;            // 确保最后一个区间的右端点是 b
	}
	local_int = Trap(local_a, local_b, local_n, h);
	MPI_Reduce(&local_int, &total_int, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	if (my_rank == 0)
	{
		cout << "\n结果是:" << 4 * total_int << endl;
		double t2 = MPI_Wtime(); /*得到当前时间t2*/
		cout << "\n时间长度是:" << t2 - t1 << " 秒" << endl;
	}
	MPI_Finalize();
	return 0;
}
double fun(double x)
{
	return 1.0 / (1.0 + pow(x, 2));
}
double Trap(double left_endpt, double right_endpt, int trap_count, double base_len)
{
	double estimate, x;
	int i;
	estimate = (fun(left_endpt) + fun(right_endpt)) / 2.0;
	for (i = 1; i <= trap_count; i++) {
		x = left_endpt + i * base_len;
		estimate += fun(x);
	}
	estimate = estimate * base_len;
	return estimate;
}


