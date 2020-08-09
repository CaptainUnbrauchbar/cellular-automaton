/* pingpong test with save timing */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "timing.h"

/* A small debugging aid. Test condition "c" and print an error if it fails */
#define Assert(c) if(!(c)){printf(\
   "\nAssertion violation %s:%u:" #c "\n", __FILE__, __LINE__);}

/* Tags */
#define PING 42
#define PONG 17

/* buffer for message exchange */
#define BUFFERSIZE 1024 * 1024
double buffer[BUFFERSIZE];

/* table of individual timings */
#define MAXREPEAT 10000
double timing[MAXREPEAT];

int main(int argc, char** argv)
{  int myId, numProcs, i, length, tries;
   double lastTime, nowTime, pa;
   MPI_Status status;

   MPI_Init(&argc, &argv);
   Assert(argc == 3);

   length = atoi(argv[1]); /* number of doubles to be send  */
   tries  = atoi(argv[2]); /* number of message exchanges   */
   Assert(length <= BUFFERSIZE);

   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myId);
   Assert(numProcs == 2);

   MPI_Barrier(MPI_COMM_WORLD);
   if (myId == 0) {
      lastTime = MPI_Wtime();
      for (i = 0;  i < tries;  i++) {
         MPI_Send(buffer, length, MPI_DOUBLE, 1, PING, MPI_COMM_WORLD);
         MPI_Recv(buffer, length, MPI_DOUBLE, 1, PONG, MPI_COMM_WORLD, &status);
         nowTime = MPI_Wtime();
         timing[i] = nowTime - lastTime;
         lastTime = nowTime;
         /* printf("%f\n", timing[i]); */
      }
   } else {
      for (i = 0;  i < tries;  i++) {
         MPI_Recv(buffer, length, MPI_DOUBLE, 0, PING, MPI_COMM_WORLD, &status);
         MPI_Send(buffer, length, MPI_DOUBLE, 0, PONG, MPI_COMM_WORLD);
      }
   }

   if (myId == 0) {
      pa = pruned_average(timing, tries, 0.25);
      printf("Average of the mid 50 percent is %f micro-s per send-receive pair\n", 
             1e6/2.0 * pa);
      printf("That corresponds to a Bandwidth of %f MByte/s\n\n", 
             2.0 * length * sizeof(double) / pa / 1024.0 / 1024.0);
   }

   MPI_Finalize(); 
}      
         
