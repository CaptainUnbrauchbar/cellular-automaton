/* Simple pingpong test without timing */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

/* A small debugging aid: test condition "c" */ 
#define Assert(c) \
  if(!(c)){printf("\nAssertion violation %s:%u:" #c "\n",\
                  __FILE__, __LINE__);}

/* tags */
#define PING 42
#define PONG 17

/* buffer for message exchange */
#define BUFFERSIZE 1024 * 1024
double buffer[BUFFERSIZE];

int main(int argc, char** argv)
{  int myId, numProcs, i, length, tries;
   MPI_Status status;

   MPI_Init(&argc, &argv);
   Assert(argc == 3);

   length = atoi(argv[1]);   /* number of doubles to be sent */
   tries  = atoi(argv[2]);   /* number of message exchanges  */
   Assert(length <= BUFFERSIZE);

   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myId);
   Assert(numProcs == 2);

   if (myId == 0) {
      for (i = 0;  i < tries;  i++) {
         MPI_Send(buffer, length, MPI_DOUBLE,
		  1, PING, MPI_COMM_WORLD);
         MPI_Recv(buffer, length, MPI_DOUBLE,
		  1, PONG, MPI_COMM_WORLD, &status);
      }
   } else {
      for (i = 0;  i < tries;  i++) {
         MPI_Recv(buffer, length, MPI_DOUBLE,
                  0, PING, MPI_COMM_WORLD, &status);
         MPI_Send(buffer, length, MPI_DOUBLE,
		  0, PONG, MPI_COMM_WORLD);
      }
   }

   MPI_Finalize(); 
}      
