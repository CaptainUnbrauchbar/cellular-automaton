/* sort integers with 
 * Quick-sort
 * and measure the required time
 * Command line arguments:
 * #2: Number of repetitions (determines timing accuracy)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "mpi.h" 

/* debugging */
/* #define NDEBUG */                   /* comment  for timing */
#include <assert.h>
#define Debug(A) A

#define MAXNPROC 512

/* machine description */
int iProc, nProc;

/* determine random integer between 0 and n-1 */
#define randInt(n) ((rand() >> 10) % (n))


/* random-number generator used for values
 * which are identical on every PE
 * The constants can be found Press Et Al.
 * Numerical Recipes in C, 2nd edition, page 284 
 */
unsigned long globalRandState;
#define sGlobalRand(s) (globalRandState = (s))
#define globalRand() (\
   globalRandState = ((1664525L*globalRandState + 1013904223L) & 0xffffffffL))
#define globalRandInt(n) ((globalRand() >> 10) % (n))


/* --------------------- debugging ----------------------------------- */

static void printItems(int *item, int size)
{  int i;
 
   for (i = 0;  i < size;  i++) {
      printf("%d ", item[i]);
   }
   if (size != 0) { printf("\n"); }
}


static void printItem(int item, MPI_Comm comm, int nP)
{  int allItems[MAXNPROC];

   MPI_Gather(&item   , 1, MPI_INT, 
              allItems, 1, MPI_INT, 0, comm);
   if (iProc == 0) {
      printf("@%d..%d:", iProc, iProc + nP - 1);
      printItems(allItems, nP);
   }
}


/* --------------------- sorting ----------------------------------- */


/* determine a pivot */
static int getPivot(int item, MPI_Comm comm, int nP)
{  int pivot   = item;
   int pivotPE = globalRandInt(nP);               /* from random PE */

   /* overwrite pivot by that one from pivotPE */
   MPI_Bcast(&pivot, 1, MPI_INT, pivotPE, comm);
   return pivot;
}


/* determine prefix-sum and overall sum over value */
static void count(int value, int *sum, int *allSum, MPI_Comm comm, int nP)
{  MPI_Scan(&value, sum, 1, MPI_INT, MPI_SUM, comm);
   *allSum = *sum;
   MPI_Bcast(allSum, 1, MPI_INT, nP - 1, comm);
   
}


/* quick-sort:sort item according to PE number in comm */
static int pQuickSort(int item, MPI_Comm comm) 
{  int iP, nP, small, allSmall;
   int pivot;
   MPI_Comm newComm;
   MPI_Status status;

   MPI_Comm_rank(comm, &iP);
   MPI_Comm_size(comm, &nP);

   if (nP == 1) { return item; }
   else {
      pivot = getPivot(item, comm, nP);
      Debug(if (iP==0) printf("pivot:%d\n", pivot);)
      count(item < pivot, &small, &allSmall, comm, nP);
      Debug(if (iP==0) printf("small:%d allSmall:%d\n", small, allSmall);)

      /* send small elements to small PE-Numbers and vice versa */ 
      if (item < pivot) {
         MPI_Bsend(&item, 1, MPI_INT, small - 1            , 88, comm);
      } else {
	 MPI_Bsend(&item, 1, MPI_INT, allSmall + iP - small, 88, comm);
      }
      
      /* receive new item */
      MPI_Recv(&item, 1, MPI_INT, MPI_ANY_SOURCE, 88, comm, &status);
      Debug(printItem(item, comm, nP));

      /* parallel recursive call */
      MPI_Comm_split(comm, iP < allSmall, 0, &newComm);
      return pQuickSort(item, newComm);
   }
} 


/* --------------------- measurement ---------------------------------- */

int main(int argc, char** argv)
{  int its, i, bufferSize;
   int item;
   double startTime, time; 
   char *buffer;

   /* init */
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nProc);
   MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
   assert(argc == 2);
   its  = atoi(argv[1]);

   /* provide enough buffering that local data can be
    * send out in buffered mode
    */
   bufferSize = sizeof(int) + MPI_BSEND_OVERHEAD;
   buffer = malloc(bufferSize);
   MPI_Buffer_attach(buffer, bufferSize);

   /* measurement loop */
   MPI_Barrier(MPI_COMM_WORLD);     /* synchronize for accurate timing */
   startTime = MPI_Wtime();   
   for (i = 1;  i <= its;  i++) {
      srand(42424 + ((iProc ^ 31415) << 10) + (i << 20));
      sGlobalRand(42424 + (i << 17));

      /* generate data */
      item = randInt(10 * nProc);
      Debug(printItem(item, MPI_COMM_WORLD, nProc));

      item = pQuickSort(item, MPI_COMM_WORLD);

      Debug(printItem(item, MPI_COMM_WORLD, nProc));
   }
   time = MPI_Wtime();
   if (iProc == 0) {
      printf("quick sort: 1 items per PE in time %fs (avg. over %d runs)\n",
	     (time - startTime) / its, its);
   }
    
   MPI_Finalize();
   return 1;
}
