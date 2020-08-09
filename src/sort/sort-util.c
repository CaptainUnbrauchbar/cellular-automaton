/* (c) 1996,1997 Peter Sanders */
/* some auxiliary routines for sorting */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "sort.h"
#include "random.h"


/* initializes key[0]..key[n-1] with random keys between 0 and bound-1
 * also works in parallel
 */
void randInitBound(int *key, int n, int bound)
{  int i, iProc, nProc;

   MPI_Comm_size(MPI_COMM_WORLD, &nProc);
   MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
   initParallelRandomLEcuyer(199773, iProc, nProc);

   for (i = 0;  i < n;  i++) {
      key[i] = (int)(nextRandomLEcuyer() * bound);
   }
}


/* ------------ some debugging aids ----------------------------- */

/* are locally present items sorted? */
static int isSorted(int *item, int size)
{  int i;
 
   for (i = 1;  i < size;  i++) {
      if (item[i - 1] > item[i]) { return 0; }
   }
   return 1;
}


/* are your items sorted and not larger than those of you neighbor? */
int isGloballySorted(int *item, int size)
{  int other, nProc, iProc;
   MPI_Status dummy;

   MPI_Comm_size(MPI_COMM_WORLD, &nProc);
   MPI_Comm_rank(MPI_COMM_WORLD, &iProc);

   MPI_Sendrecv(&(item[0]), 1, MPI_INT, (iProc - 1 + nProc) % nProc, 42,
                &other    , 1, MPI_INT, (iProc + 1        ) % nProc, 42,
                MPI_COMM_WORLD, &dummy);  
 
   return isSorted(item, size) && 
          (iProc == nProc - 1 || item[size - 1] <= other); 
}


/* maximum number of elements per PE which can be printed */
#define MAXPRINT 1024
/* gather all items at PE 0 and print them */
void printItemsGlobally(int *item, int size)
{  int i, j, nProc, iProc;
   MPI_Status status;
   int buffer[MAXPRINT];
 
   MPI_Comm_size(MPI_COMM_WORLD, &nProc);
   MPI_Comm_rank(MPI_COMM_WORLD, &iProc);

   
   if (iProc == 0) {
      for (i = 0;  i < size;  i++) {
	 printf("%d ", item[i]);
      }
      for (j = 1;  j < nProc;  j++) {
	 MPI_Recv(buffer, MAXPRINT, MPI_INT, j, 77, MPI_COMM_WORLD, &status);
         MPI_Get_count(&status, MPI_INT, &size);
	 for (i = 0;  i < size;  i++) {
	    printf("%d ", buffer[i]);
	 }
      }
      printf("\n");
   } else {
      MPI_Send(item,  size,  MPI_INT, 0, 77, MPI_COMM_WORLD);
   }
}


/* ------------------ comparison function for quicksort ----------------- */

int intCmp(const void *a, const void *b)
{  return (*(int*)a < *(int*)b  ?  -1  :  *(int*)a > *(int*)b);
} 


/*------------ functions useful for quicksort and sample sort ----------- */

/* rearrange item[0]..item[size-1] according to pivot.
 * return an index i such that 
 * for all i <= j < size: item[j] >= pivot and
 * for all 0 <= j < i   : item[j] <= pivot 
 * the algorithm used is similar to the Quicksort partition 
 * algorithm described in "Duden Informatik"
 */
int split(int *item, int size, int pivot)
{  int i, j;
   int temp;

   i = 0;
   j = size - 1;      
   do {
      while (i < size && item[i] < pivot) { i++; }
      while (j >= 0   && item[j] > pivot) { j--; }
      if (i <= j) {
         temp = item[i];  item[i] = item[j];  item[j] = temp;
         i++; j--;
      }
   } while (i <= j);
   return i;
}


/* partition item[0]..item[size-1] into k parts 
 * according to pivot[1]..pivot[k-1] such that
 * forall 0 <= i < k 
 *    forall start[i] <= x < start[i+i]
 *       pivot[i] <= *x <= pivot[i+1]
 * (assume pivot[0] = -infinity, pivot[k] = infinity) 
 * pivot[0] and pivot[k] are never really accessed
 */ 
void partition(int *item, int size, int *pivot, int **start, int k)
{  int splitPos;

   if (k == 1) {    
      start[0] = item;
      start[1] = item + size;
   } else {
      splitPos = split(item, size, pivot[k/2]);
      partition(item         , splitPos     , pivot    , start    , k/2);
      partition(item+splitPos, size-splitPos, pivot+k/2, start+k/2, k-k/2);
   }
}
