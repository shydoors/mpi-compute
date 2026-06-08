#include <inttypes.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
/**规约
 * 将消息集中起来处理，与广播刚好相反
 * MPI_Reduce
 */
int32_t main(int argc, char *argv[]) {
  int32_t rank, size;
  int64_t data_a, data_b;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("rank == %" PRId32 " \n", rank);
  data_b = 0;
  data_a = rank + 10;
  MPI_Reduce(&data_a, &data_b, 1, MPI_INT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
  /**
   * 问：
   * 怎么理解MPI_Reduce函数的 sendbuf , recvbuf 参数?
   * 规约是将消息集中起来处理，与广播刚好相反
   * 那么数据理论上只需要把东西赋值给 sendbuf 就行了啊？
   *
   * 答：
   * ===> 需要贡献的数据通过 sendbuf 送出，结果只出现在了根进程的 recvbuf 里。
   */
  if (0 == rank) {
    printf("process %" PRId32 " total %" PRId64 " \n", rank, data_b);
  }
  MPI_Finalize();
  return 0;
}