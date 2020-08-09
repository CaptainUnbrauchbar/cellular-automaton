#include <stdio.h>
#include "mpi.h"

int main(int argc, char** argv)
{
   int myId, numProcs;

   MPI_Init(&argc, &argv);

   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myId);

   printf("I am PE %d out of %d PEs.\n", myId, numProcs);

   MPI_Finalize();
}
