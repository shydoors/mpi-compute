#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);
	MPI_Comm comm = MPI_COMM_WORLD;
	int rank, size, part, sbuf[3], *rbuf, i;
	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);
	MPI_Status status;
	for (i = 0; i < 3; i++)
		sbuf[i] = rank * 10 + i;
	rbuf = (int*)malloc(sizeof(int) * 3 * size);
	MPI_Allgather(sbuf, 3, MPI_INT, rbuf, 3, MPI_INT, comm);
	printf("process %d recv :", rank);
	for (i = 0; i < 3 * size; i++)
		printf(" %d", rbuf[i]);
	putchar('\n');
	free(rbuf);
	MPI_Finalize();
	return 0;
}