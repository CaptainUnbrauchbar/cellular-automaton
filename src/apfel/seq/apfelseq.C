/* (c) 1997 Ingo Boesnach */
/* mandelbrot set (serial version)
 * argv[1]: smallest real part considered
 * argv[2]: smallest imaginary part considered
 * argv[3]: extent of square area considered
 * argv[4]: resolution 
 * argv[5]: maximum number of interations
 * argv[6]: display 
 * e.g. "poe a.out -procs 2 -1.5 -1 2 400 200 i90s14.ira.uka.de:0.0"
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <assert.h>
#include "mpi.h"
#include "random.h"
#include "bitmap.h"

#define DISPLAY   1

int iProc, nProc;

typedef struct {int start;  int end; } Work;

/*********** Mandelbrot specific code ******************************/

#define LARGE   2.0

complex z0;
double extent;  /* a_0 in the book         */
int resolution; /* n in the book           */
double step;    /* a_0/n                   */
int maxiter;    /* m_max in the book       */
int withgraph;  /* Flag: display results ? */

/* perform a single Mandelbrot iteration starting at c and return
 * number of iterations
 */
int iterate(int pos)
{  
  int iter;
  complex c = z0 + complex((double)(pos % resolution) * step, 
			   (double)(pos / resolution) * step);
  complex z = c;
  for (iter = 1;  iter < maxiter && abs(z) <= LARGE;  iter++) z = z*z + c;
  return iter;
}


/*********** Server Code **********************/

void server(char *display)
{ int i, x, y, length;
  MPI_Status status;
  int *recvBuffer = (int*)malloc(resolution * resolution * sizeof(int));

  assert(recvBuffer != 0);
  if(withgraph) bitmapInit(resolution, resolution, display);
  
  /* Get an interval size and a sequence of integers 
   * encoding one element of the Mandelbrot set each.
   * The interval size can be ignored here.
   * But we need it in some parallel codes
   * using the same data format.
   */
  MPI_Recv(recvBuffer, resolution*resolution, MPI_INT, 
	   MPI_ANY_SOURCE, DISPLAY, MPI_COMM_WORLD, &status);
  MPI_Get_count(&status, MPI_INT, &length);

  printf("plotting %d pixels out of %d\n", length-1, recvBuffer[0]);
  for (i = 1;  i < length;  i++) {
    x = recvBuffer[i] % resolution;
    y = recvBuffer[i] / resolution;
    if(withgraph) bitmapSetPixel(x, resolution - y - 1);
    /* remember: (0,0) of bitmap corresponds to complex (z0 + i*n) */
  }
  if(withgraph) bitmapDisplay();
  free(recvBuffer);
}


/*********** Worker Code **********************/

/* work on an interval of integers representing candidate elements
 * of the Mandelbrot set.
 */
void worker(Work work)
{ 
  int elem;
  int *result;
  /* contains interval size and a sequence of integers encoding one element of 
   * the Mandelbrot set each 
   */
  result = (int*)malloc(resolution * resolution * sizeof(int) + 1);
  assert(result != 0);
  
  elem = 1;
  result[0] = work.start; /* remember where this interval started */
  printf("starting work on %d to %d\n", work.start, work.end);
  for (; work.start <= work.end;  work.start++) {
    if (iterate(work.start) == maxiter) {
      /* we assume to have an element of the Mandelbrot set here */
      result[elem] = work.start;
      elem++;
    }
  }
  /* send results to server */
  result[0] = work.end - result[0] + 1;
  MPI_Send(result, elem, MPI_INT, 0, DISPLAY, MPI_COMM_WORLD);  

  free(result);
}


/*********************************************************************/

void main(int argc, char **argv)
{ Work work;
  
  MPI_Init(&argc, &argv);
  assert(argc >= 6);
  assert(argc <= 7);
  z0         = complex(atof(argv[1]), atof(argv[2]));
  extent     = atof(argv[3]);
  resolution = atoi(argv[4]);
  step = extent / resolution;
  maxiter = atoi(argv[5]);
  if(argc == 6) withgraph = 0; else withgraph = 1;
  
  MPI_Comm_size(MPI_COMM_WORLD, &nProc);
  assert(nProc == 2);
  MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
  
  if(iProc == 0) server(argv[6]);
  else {
    work.start = 0; 
    work.end = resolution*resolution - 1;
    worker(work);
  }
  
  getchar();
  MPI_Finalize();
}
