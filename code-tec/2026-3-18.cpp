//传递结构体
#include <iostream>
#include <mpi.h>

class Point {
public:
	double x;
	double y;

	Point(double x_val, double y_val) : x(x_val), y(y_val) {}
};

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// Define MPI data type for Point
	MPI_Datatype point_type;
	int block_lengths[2] = { 1, 1 };
	MPI_Datatype types[2] = { MPI_DOUBLE, MPI_DOUBLE };
	MPI_Aint offsets[2];
	offsets[0] = offsetof(Point, x);
	offsets[1] = offsetof(Point, y);
	MPI_Type_create_struct(2, block_lengths, offsets, types, &point_type);
	MPI_Type_commit(&point_type);

	// Example usage: sending a Point instance from rank 0 to rank 1
	if (rank == 0) {
		Point p(3.5, 2.0);
		MPI_Send(&p, 1, point_type, 1, 0, MPI_COMM_WORLD);
	}
	else if (rank == 1) {
		Point received_point(0.0, 0.0);
		MPI_Recv(&received_point, 1, point_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		std::cout << "Received Point: (" << received_point.x << ", " << received_point.y << ")" << std::endl;
	}

	MPI_Finalize();
	return 0;
}