/* parallel odd-even sort. One (random) element per PE
 * and measure the required time
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h" 

/* debugging */
#include <assert.h>
#define Debug(A) A

/* machine description */
int iProc, nProc;

#define sync() MPI_Barrier(MPI_COMM_WORLD)

/* --------------------- sorting ----------------------------------- */

/* sort nProc integers. one from each PE
 */
#define sendInt(n, p) MPI_Ssend(&n, 1, MPI_INT, p, 42, MPI_COMM_WORLD);
#define recvInt(n, p) MPI_Recv (&n, 1, MPI_INT, p, 42, MPI_COMM_WORLD, &dummy);
MPI_Status dummy;
static int oddEvenSort(int item) 
{  int i; 
   int otherItem;

   for (i = 0;  i <= nProc;  i++) {
      if ((iProc & 1) == (i & 1)) {
	 if (iProc != 0) {
            /* exchange data item with left neighbor */
            sendInt(item     , iProc - 1);
            recvInt(otherItem, iProc - 1);
            if (item < otherItem) { item = otherItem; }
	 }
      } else {
	 if (iProc + 1 < nProc) {
            /* exchange data item with right neighbor */
            recvInt(otherItem, iProc + 1);
            sendInt(item     , iProc + 1);
            if (otherItem < item) { item = otherItem; }
	 }
      }
   }
   return item;
} 


/* ----------------------- debugging ---------------------------------- */

static void printItems(int *item, int size)
{  int i;
 
   for (i = 0;  i < size;  i++) {
      printf("%d ", item[i]);
   }
   if (size != 0) { printf("\n"); }
}


static void printItem(int item)
{  int allItems[1024];

   sync();
   MPI_Gather(&item   , 1, MPI_INT, 
              allItems, 1, MPI_INT, 0, MPI_COMM_WORLD);
   if (iProc == 0) {
      printItems(allItems, nProc);
   }
   sync();
}


/* --------------------- measurement ---------------------------------- */

int main(int argc, char** argv)
{  int its, i;
   int item;
   double startTime, time; 

   /* init */
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nProc);
   MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
   assert(argc == 2);
   its = atoi(argv[1]);

   /* measurement loop */
   srand(4242 + iProc * 3141);
   sync();                           /* synchronize for accurate timing */
   startTime = MPI_Wtime();   
   for (i = 1;  i <= its;  i++) {
      item = rand();
      Debug(printItem(item));
      item = oddEvenSort(item);
      Debug(printItem(item));
   }
   time = MPI_Wtime();
   if (iProc == 0) {
      printf(
         "\nodd-even sort: on %d PEs in time %fs (avg. over %d runs)\n",
	  nProc, (time - startTime) / its, its);
   }
    
   MPI_Finalize();
   return 1;
}
