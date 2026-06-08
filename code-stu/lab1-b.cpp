#include <stdio.h>
#include <omp.h>
int main() {
    long sum_crit = 0, sum_red = 0;
    int N = 100000;
    // 方式1：临界区 (串行化累加，安全但较慢)
    #pragma omp parallel for
    for (int i = 1; i <= N; i++) {
        #pragma omp critical
        sum_crit += i;
    }
    // 方式2：规约子句 (编译器自动分块累加，高效)
    #pragma omp parallel for reduction(+:sum_red)
    for (int i = 1; i <= N; i++) {
        sum_red += i;
    }
    printf("Critical结果: %ld\nReduction结果: %ld\n理论值: %ld\n",
           sum_crit, sum_red, (long)N * (N + 1) / 2);
    return 0;
}