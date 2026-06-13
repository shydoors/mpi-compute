/**
 * @file 2026-3-7.cpp
 * @brief MPI 集合通信求定积分
 */
#include <cmath>
#include <mpi.h>
#include <iostream>
double fun(double x) {
	return 1.0 / (1.0 + pow(x, 2));
}
double Trap(double left_endpt, double right_endpt, int trap_count, double base_len) {
	double estimate, x;
	estimate = (fun(left_endpt) + fun(right_endpt)) / 2.0;
    for (int i = 1; i <= trap_count; i++) {
		x = left_endpt + i * base_len;
		estimate += fun(x);
	}
	estimate = estimate * base_len;
	return estimate;
}

int main(int argc, char *argv[]) {
    int my_rank, comm_sz, n, local_n;
    double a, b, h, local_a, local_b, local_int, total_int;
    a = 0;
    b = 1.0;
    n = 1024 * 1024 * 1024;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    double t1 = MPI_Wtime();
    h = (b - a) / (double)n;
    local_n = n / comm_sz;
    local_a = a + my_rank * local_n * h;
    local_b = local_a + local_n * h;
    if (my_rank == comm_sz - 1) {
        local_n += n % comm_sz;
        local_b = b;
    }
    local_int = Trap(local_a, local_b, local_n, h);

    MPI_Reduce(&local_int, &total_int, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        std::cout << "\n结果是:" << 4 * total_int << std::endl;
        double t2 = MPI_Wtime();
        std::cout << "\n时间长度是:" << t2 - t1 << " 秒" << std::endl;
    }
    MPI_Finalize();
    return 0;
}

