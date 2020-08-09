/* pingpong test with timing vor varying message sizes 
 * June 4 97: version with pruned_average
 *
 * (c) 1996,1997 Peter Sanders
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
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

/* each measurement should take about 1s */
#define NTries(length) (1.0 / (50e-6 + sizeof(double) * (length) * 1e-8))

int main(int argc, char** argv)
{  int myId, numProcs, i, length, tries;
   double lastTime, nowTime, pa, *timing;
   MPI_Status status;

   MPI_Init(&argc, &argv);

   length = 1;

   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myId);
   Assert(numProcs == 2);

   timing = malloc(NTries(1) * sizeof(double));
   Assert(timing != 0);

   for (length = 1;  length <= BUFFERSIZE;  length *= 2) {
      tries = NTries(length);
      /* if (myId == 0) printf("%d tries\n", tries); */
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
         printf("%d %g %g # length, time[us], bandwidth [MB/s] %d tries\n",
                length * sizeof(double),
                1e6 * pa / 2.0,
                2.0 * length * sizeof(double) / pa / 1024.0 / 1024.0, 
                tries);
      }
   }
   MPI_Finalize(); 
}      
         
