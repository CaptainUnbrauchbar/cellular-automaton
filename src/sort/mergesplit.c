/* sort a distributed array of (random) integers with 
 * merge-splitting sort
 * and measure the required time
 * Command line arguments:
 * #1: Number of values per PE
 * #2: Number of repetitions (determines timing accuracy)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "mpi.h" 
#include "sort.h"

/* debugging */
/* #define NDEBUG */                  /* uncomment  for timing */
#include <assert.h>
#define Debug(A) A              /* comment out body for timing */

/* memory for data items */
#define MAXDATA (256 * 1024)
int item       [MAXDATA + 1];

/* machine description */
int iProc, nProc;


/* merge s1 values form a1 and s2 values from a2 to b 
 * in a1 and a2 there must be one extra free entry for a sentinel
 */
void merge(int *a1, int s1, int *a2, int s2, int *b)
{  int i;
   
   a1[s1] = a2[s2] = INT_MAX;                       /* add sentinels */
   for (i = 0;  i < s1 + s2;  i++, b++) {
      if (*a1 <= *a2) { *b = *a1;  a1++; } 
      else            {	*b = *a2;  a2++; }
   }  
}


int inBuffer   [    MAXDATA + 1];
int mergeBuffer[2 * MAXDATA    ];

/* sort elements from each PE in comm.
 * comm may only represent a subset of all PEs.
 * size gives the number of values in item which are to be sorted
 * size's  value may differ from PE to PE
 * the portion of sorted values present at PE iProc
 * is output in item[0]..item[*outSize - 1]
 */
static void mergeSplitSort(int * item, int size, int *outSize, MPI_Comm comm) 
{  int iP, nP;                                      /* rank and size of comm */
   MPI_Status status;
   int i;
   int inSize, totalSize;                   /* received/total amount of data */

/* shorthands */
#define sendItems(b, p) MPI_Ssend(b, size   , MPI_INT, p, 42, comm)
#define recvItems(b, p) MPI_Recv (b, MAXDATA, MPI_INT, p, 42, comm, &status)

   MPI_Comm_size(comm, &nP);
   MPI_Comm_rank(comm, &iP);

   /* sort locally */
   intSort(item, size);

   /* iteration scheme analogous to odd-even sort */
   for (i = 1;  i <= nP;  i++) {
      if ((iP & 1) == (i & 1)) {
	 if (iP != 0) {
            /* exchange data with left neighbor */
            Debug(printf("%d:%d<->%d\n", i, iP, iP-1));
            sendItems(item    , iP - 1);
            recvItems(inBuffer, iP - 1);
            MPI_Get_count(&status, MPI_INT, &inSize);
            totalSize = size + inSize;

            /* merge and keep larger half */
            merge(item, size, inBuffer, inSize, mergeBuffer);
            size = totalSize - totalSize/2;
            memcpy(item, mergeBuffer + totalSize/2, size * sizeof(int));
	 }
      } else {
	 if (iP + 1 < nP) {
            /* exchange data with right neighbor */
            Debug(printf("%d:%d<->%d\n", i, iP, iP+1));
            recvItems(inBuffer, iP + 1);
            sendItems(item    , iP + 1);
            MPI_Get_count(&status, MPI_INT, &inSize);
            totalSize = size + inSize;

            /* merge and keep smaller half */
            merge(item, size, inBuffer, inSize, mergeBuffer);
            size = totalSize/2;
            memcpy(item, mergeBuffer, size * sizeof(int));
	 }
      }
      Debug(printItemsGlobally(mergeBuffer, 2*size));
   }
   *outSize = size;
} 


/* --------------------- measurement ---------------------------------- */

int main(int argc, char** argv)
{  int size, its, i, j, dummy;
   double startTime, time; 

   /* init */
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nProc);
   MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
   assert(argc == 3);
   size = atoi(argv[1]);
   its  = atoi(argv[2]);
   assert(size <= MAXDATA);

   /* measurement loop */
   srand(42424 + ((iProc ^ 31415) << 10));
   MPI_Barrier(MPI_COMM_WORLD);     /* synchronize for accurate timing */
   startTime = MPI_Wtime();   
   for (i = 1;  i <= its;  i++) {

      /* generate data */
      for (j = 0;  j < size;  j++) {
         item[j] = rand();
      }
      Debug(printItemsGlobally(item, size));

      mergeSplitSort(item, size, &dummy, MPI_COMM_WORLD);

      Debug(printItemsGlobally(item, size));
      assert(isGloballySorted(item, size));
   }
   time = MPI_Wtime();
   if (iProc == 0) {
      printf("merge-split sort: %d items per PE in time %fs (avg. over %d runs)\n",
	     size, (time - startTime) / its, its);
   }
    
   MPI_Finalize();
   return 1;
}
